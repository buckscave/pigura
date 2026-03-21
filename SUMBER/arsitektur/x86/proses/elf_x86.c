/*
 * PIGURA OS - ELF LOADER x86
 * ---------------------------
 * Implementasi ELF binary loader untuk arsitektur x86.
 *
 * Berkas ini berisi fungsi-fungsi untuk memuat dan
 * mengeksekusi file ELF.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA ELF
 * ============================================================================
 */

/* ELF magic */
#define ELF_MAGIC               0x464C457F

/* ELF class */
#define ELF_CLASS_32            1
#define ELF_CLASS_64            2

/* ELF data encoding */
#define ELF_DATA_LSB            1
#define ELF_DATA_MSB            2

/* ELF type */
#define ET_NONE                 0
#define ET_REL                  1
#define ET_EXEC                 2
#define ET_DYN                  3
#define ET_CORE                 4

/* ELF machine */
#define EM_386                  3
#define EM_X86_64               62

/* Program header type */
#define PT_NULL                 0
#define PT_LOAD                 1
#define PT_DYNAMIC              2
#define PT_INTERP               3
#define PT_NOTE                 4
#define PT_SHLIB                5
#define PT_PHDR                 6

/* Program header flags */
#define PF_X                    0x01
#define PF_W                    0x02
#define PF_R                    0x04

/*
 * ============================================================================
 * STRUKTUR ELF
 * ============================================================================
 */

/* ELF header 32-bit */
struct elf32_header {
    tak_bertanda8_t e_ident[16];
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
} __attribute__((packed));

/* Program header 32-bit */
struct elf32_phdr {
    tak_bertanda32_t p_type;
    tak_bertanda32_t p_offset;
    tak_bertanda32_t p_vaddr;
    tak_bertanda32_t p_paddr;
    tak_bertanda32_t p_filesz;
    tak_bertanda32_t p_memsz;
    tak_bertanda32_t p_flags;
    tak_bertanda32_t p_align;
} __attribute__((packed));

/* Section header 32-bit */
struct elf32_shdr {
    tak_bertanda32_t sh_name;
    tak_bertanda32_t sh_type;
    tak_bertanda32_t sh_flags;
    tak_bertanda32_t sh_addr;
    tak_bertanda32_t sh_offset;
    tak_bertanda32_t sh_size;
    tak_bertanda32_t sh_link;
    tak_bertanda32_t sh_info;
    tak_bertanda32_t sh_addralign;
    tak_bertanda32_t sh_entsize;
} __attribute__((packed));

/* Symbol table entry */
struct elf32_sym {
    tak_bertanda32_t st_name;
    tak_bertanda32_t st_value;
    tak_bertanda32_t st_size;
    tak_bertanda8_t st_info;
    tak_bertanda8_t st_other;
    tak_bertanda16_t st_shndx;
} __attribute__((packed));

/* Relocation entry */
struct elf32_rel {
    tak_bertanda32_t r_offset;
    tak_bertanda32_t r_info;
} __attribute__((packed));

struct elf32_rela {
    tak_bertanda32_t r_offset;
    tak_bertanda32_t r_info;
    tak_bertanda32_t r_addend;
} __attribute__((packed));

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* Error string */
static char g_elf_error[256];

/*
 * ============================================================================
 * FUNGSI INTERNAL
 * ============================================================================
 */

/*
 * _validasi_header
 * ----------------
 * Memvalidasi header ELF.
 *
 * Parameter:
 *   header - Pointer ke ELF header
 *
 * Return:
 *   STATUS_BERHASIL jika valid
 */
static status_t _validasi_header(struct elf32_header *header)
{
    tak_bertanda32_t magic;

    if (header == NULL) {
        kernel_strncpy(g_elf_error, "Header NULL", 255);
        return STATUS_PARAM_INVALID;
    }

    /* Cek magic */
    magic = *(tak_bertanda32_t *)header->e_ident;

    if (magic != ELF_MAGIC) {
        kernel_strncpy(g_elf_error, "Magic tidak valid", 255);
        return STATUS_GAGAL;
    }

    /* Cek class (32-bit) */
    if (header->e_ident[4] != ELF_CLASS_32) {
        kernel_strncpy(g_elf_error, "Bukan ELF 32-bit", 255);
        return STATUS_GAGAL;
    }

    /* Cek endianness */
    if (header->e_ident[5] != ELF_DATA_LSB) {
        kernel_strncpy(g_elf_error, "Bukan little-endian", 255);
        return STATUS_GAGAL;
    }

    /* Cek machine */
    if (header->e_machine != EM_386) {
        kernel_strncpy(g_elf_error, "Bukan i386", 255);
        return STATUS_GAGAL;
    }

    /* Cek type (executable) */
    if (header->e_type != ET_EXEC) {
        kernel_strncpy(g_elf_error, "Bukan executable", 255);
        return STATUS_GAGAL;
    }

    return STATUS_BERHASIL;
}

/*
 * _muat_segment
 * -------------
 * Memuat satu segmen ELF.
 *
 * Parameter:
 *   data    - Pointer ke data file
 *   phdr    - Pointer ke program header
 *   vaddr   - Alamat virtual tujuan
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
static status_t _muat_segment(void *data, struct elf32_phdr *phdr,
                               alamat_virtual_t *vaddr)
{
    void *src;
    void *dst;

    if (data == NULL || phdr == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Skip non-load segments */
    if (phdr->p_type != PT_LOAD) {
        return STATUS_BERHASIL;
    }

    /* Get source dan destination */
    src = (void *)((tak_bertanda8_t *)data + phdr->p_offset);
    dst = (void *)phdr->p_vaddr;

    kernel_printf("[ELF] Memuat segmen: 0x%08x -> 0x%08x (%u bytes)\n",
                  phdr->p_vaddr, phdr->p_vaddr + phdr->p_memsz,
                  phdr->p_filesz);

    /* Map pages untuk segmen ini */
    /* Untuk sekarang, kita asumsikan memory sudah di-map */
    /* Copy file content */
    if (phdr->p_filesz > 0) {
        kernel_memcpy(dst, src, phdr->p_filesz);
    }

    /* Zero-fill remainder (BSS) */
    if (phdr->p_memsz > phdr->p_filesz) {
        kernel_memset((void *)((tak_bertanda8_t *)dst + phdr->p_filesz),
                      0, phdr->p_memsz - phdr->p_filesz);
    }

    if (vaddr != NULL) {
        *vaddr = phdr->p_vaddr;
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * elf_muat
 * --------
 * Memuat file ELF dari buffer.
 *
 * Parameter:
 *   buffer - Pointer ke data ELF
 *   ukuran - Ukuran buffer
 *   entry  - Pointer untuk menyimpan entry point
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t elf_muat(void *buffer, ukuran_t ukuran,
                   alamat_virtual_t *entry)
{
    struct elf32_header *header;
    struct elf32_phdr *phdr;
    tak_bertanda32_t i;
    status_t status;

    if (buffer == NULL || ukuran < sizeof(struct elf32_header)) {
        return STATUS_PARAM_INVALID;
    }

    header = (struct elf32_header *)buffer;

    /* Validasi header */
    status = _validasi_header(header);
    if (status != STATUS_BERHASIL) {
        kernel_printf("[ELF] Validasi gagal: %s\n", g_elf_error);
        return status;
    }

    /* Cek apakah ada program header */
    if (header->e_phnum == 0 || header->e_phoff == 0) {
        kernel_strncpy(g_elf_error, "Tidak ada program header", 255);
        return STATUS_GAGAL;
    }

    /* Get program header array */
    phdr = (struct elf32_phdr *)((tak_bertanda8_t *)buffer + 
             header->e_phoff);

    /* Muat semua segmen */
    for (i = 0; i < header->e_phnum; i++) {
        status = _muat_segment(buffer, &phdr[i], NULL);
        if (status != STATUS_BERHASIL) {
            kernel_printf("[ELF] Gagal memuat segmen %u\n", i);
            return status;
        }
    }

    /* Return entry point */
    if (entry != NULL) {
        *entry = header->e_entry;
    }

    kernel_printf("[ELF] ELF dimuat, entry: 0x%08x\n", header->e_entry);

    return STATUS_BERHASIL;
}

/*
 * elf_muat_dari_file
 * ------------------
 * Memuat file ELF dari filesystem.
 *
 * Parameter:
 *   path   - Path ke file
 *   entry  - Pointer untuk menyimpan entry point
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t elf_muat_dari_file(const char *path, alamat_virtual_t *entry)
{
    /* Placeholder - perlu VFS */
    kernel_printf("[ELF] Muat dari file: %s (belum implementasi)\n", path);

    return STATUS_TIDAK_DUKUNG;
}

/*
 * elf_get_error
 * -------------
 * Mendapatkan string error terakhir.
 *
 * Return:
 *   String error
 */
const char *elf_get_error(void)
{
    return g_elf_error;
}

/*
 * elf_get_entry
 * ------------
 * Mendapatkan entry point dari header.
 *
 * Parameter:
 *   buffer - Pointer ke data ELF
 *
 * Return:
 *   Entry point
 */
alamat_virtual_t elf_get_entry(void *buffer)
{
    struct elf32_header *header;

    if (buffer == NULL) {
        return 0;
    }

    header = (struct elf32_header *)buffer;

    return (alamat_virtual_t)header->e_entry;
}

/*
 * elf_get_program_headers
 * -----------------------
 * Mendapatkan array program header.
 *
 * Parameter:
 *   buffer - Pointer ke data ELF
 *   count  - Pointer untuk menyimpan jumlah
 *
 * Return:
 *   Pointer ke array program header
 */
void *elf_get_program_headers(void *buffer, tak_bertanda32_t *count)
{
    struct elf32_header *header;

    if (buffer == NULL || count == NULL) {
        return NULL;
    }

    header = (struct elf32_header *)buffer;

    *count = header->e_phnum;

    return (void *)((tak_bertanda8_t *)buffer + header->e_phoff);
}

/*
 * elf_get_memory_size
 * -------------------
 * Menghitung total ukuran memori yang diperlukan.
 *
 * Parameter:
 *   buffer - Pointer ke data ELF
 *
 * Return:
 *   Ukuran total dalam byte
 */
ukuran_t elf_get_memory_size(void *buffer)
{
    struct elf32_header *header;
    struct elf32_phdr *phdr;
    tak_bertanda32_t i;
    tak_bertanda32_t max_addr;
    tak_bertanda32_t min_addr;
    ukuran_t total;

    if (buffer == NULL) {
        return 0;
    }

    header = (struct elf32_header *)buffer;
    phdr = (struct elf32_phdr *)((tak_bertanda8_t *)buffer + 
             header->e_phoff);

    max_addr = 0;
    min_addr = 0xFFFFFFFF;

    for (i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            tak_bertanda32_t end = phdr[i].p_vaddr + phdr[i].p_memsz;

            if (phdr[i].p_vaddr < min_addr) {
                min_addr = phdr[i].p_vaddr;
            }
            if (end > max_addr) {
                max_addr = end;
            }
        }
    }

    total = (ukuran_t)(max_addr - min_addr);

    return total;
}

/*
 * elf_is_valid
 * ------------
 * Cek apakah buffer adalah file ELF valid.
 *
 * Parameter:
 *   buffer - Pointer ke data
 *   ukuran - Ukuran buffer
 *
 * Return:
 *   BENAR jika valid
 */
bool_t elf_is_valid(void *buffer, ukuran_t ukuran)
{
    struct elf32_header *header;

    if (buffer == NULL || ukuran < sizeof(struct elf32_header)) {
        return SALAH;
    }

    header = (struct elf32_header *)buffer;

    return (_validasi_header(header) == STATUS_BERHASIL) ? BENAR : SALAH;
}

/*
 * elf_print_info
 * --------------
 * Menampilkan informasi ELF.
 *
 * Parameter:
 *   buffer - Pointer ke data ELF
 */
void elf_print_info(void *buffer)
{
    struct elf32_header *header;
    struct elf32_phdr *phdr;
    tak_bertanda32_t i;

    if (buffer == NULL) {
        return;
    }

    header = (struct elf32_header *)buffer;

    kernel_printf("[ELF] Informasi ELF:\n");
    kernel_printf("[ELF]   Type: %u\n", header->e_type);
    kernel_printf("[ELF]   Machine: %u\n", header->e_machine);
    kernel_printf("[ELF]   Entry: 0x%08x\n", header->e_entry);
    kernel_printf("[ELF]   Program headers: %u\n", header->e_phnum);

    phdr = (struct elf32_phdr *)((tak_bertanda8_t *)buffer + 
             header->e_phoff);

    kernel_printf("[ELF]   Segmen:\n");

    for (i = 0; i < header->e_phnum; i++) {
        kernel_printf("[ELF]     [%u] type=%u addr=0x%08x "
                      "size=%u memsz=%u\n",
            i, phdr[i].p_type, phdr[i].p_vaddr,
            phdr[i].p_filesz, phdr[i].p_memsz);
    }
}

