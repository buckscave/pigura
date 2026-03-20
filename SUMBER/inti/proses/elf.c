/*
 * PIGURA OS - ELF.C
 * ------------------
 * Implementasi loader berkas ELF.
 *
 * Berkas ini berisi fungsi-fungsi untuk memuat dan memproses
 * berkas executable dalam format ELF (Executable and Linkable Format).
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* ELF Magic */
#define ELF_MAGIC               0x464C457F  /* 0x7F 'E' 'L' 'F' */

/* ELF Class */
#define ELF_CLASS_32            1
#define ELF_CLASS_64            2

/* ELF Data encoding */
#define ELF_DATA_LSB            1    /* Little endian */
#define ELF_DATA_MSB            2    /* Big endian */

/* ELF Type */
#define ELF_TYPE_NONE           0
#define ELF_TYPE_REL            1    /* Relocatable */
#define ELF_TYPE_EXEC           2    /* Executable */
#define ELF_TYPE_DYN            3    /* Shared object */
#define ELF_TYPE_CORE           4    /* Core file */

/* ELF Machine */
#define ELF_MACHINE_386         3    /* i386 */
#define ELF_MACHINE_X86_64      62   /* x86_64 */
#define ELF_MACHINE_ARM         40   /* ARM */
#define ELF_MACHINE_AARCH64     183  /* ARM64 */

/* ELF Program header type */
#define PT_NULL                 0
#define PT_LOAD                 1    /* Loadable segment */
#define PT_DYNAMIC              2    /* Dynamic linking */
#define PT_INTERP               3    /* Interpreter */
#define PT_NOTE                 4    /* Notes */
#define PT_SHLIB                5    /* Shared library */
#define PT_PHDR                 6    /* Program header */
#define PT_GNU_STACK            0x6474E551

/* Program header flags */
#define PF_X                    0x01  /* Executable */
#define PF_W                    0x02  /* Writable */
#define PF_R                    0x04  /* Readable */

/* ELF Section header type */
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

/*
 * ============================================================================
 * STRUKTUR DATA ELF (ELF DATA STRUCTURES)
 * ============================================================================
 */

/* ELF Identification */
typedef struct {
    tak_bertanda8_t e_ident[16];    /* ELF identification */
} elf_ident_t;

/* ELF32 Header */
typedef struct {
    tak_bertanda8_t  e_ident[16];   /* ELF identification */
    tak_bertanda16_t e_type;        /* Object file type */
    tak_bertanda16_t e_machine;     /* Machine type */
    tak_bertanda32_t e_version;     /* Object file version */
    tak_bertanda32_t e_entry;       /* Entry point address */
    tak_bertanda32_t e_phoff;       /* Program header offset */
    tak_bertanda32_t e_shoff;       /* Section header offset */
    tak_bertanda32_t e_flags;       /* Processor-specific flags */
    tak_bertanda16_t e_ehsize;      /* ELF header size */
    tak_bertanda16_t e_phentsize;   /* Size of program header entry */
    tak_bertanda16_t e_phnum;       /* Number of program header entries */
    tak_bertanda16_t e_shentsize;   /* Size of section header entry */
    tak_bertanda16_t e_shnum;       /* Number of section header entries */
    tak_bertanda16_t e_shstrndx;    /* Section name string table index */
} elf32_header_t;

/* ELF32 Program header */
typedef struct {
    tak_bertanda32_t p_type;        /* Type of segment */
    tak_bertanda32_t p_offset;      /* Offset in file */
    tak_bertanda32_t p_vaddr;       /* Virtual address in memory */
    tak_bertanda32_t p_paddr;       /* Physical address */
    tak_bertanda32_t p_filesz;      /* Size of segment in file */
    tak_bertanda32_t p_memsz;       /* Size of segment in memory */
    tak_bertanda32_t p_flags;       /* Segment flags */
    tak_bertanda32_t p_align;       /* Alignment of segment */
} elf32_phdr_t;

/* ELF32 Section header */
typedef struct {
    tak_bertanda32_t sh_name;       /* Section name */
    tak_bertanda32_t sh_type;       /* Section type */
    tak_bertanda32_t sh_flags;      /* Section flags */
    tak_bertanda32_t sh_addr;       /* Virtual address */
    tak_bertanda32_t sh_offset;     /* Offset in file */
    tak_bertanda32_t sh_size;       /* Size of section */
    tak_bertanda32_t sh_link;       /* Link to another section */
    tak_bertanda32_t sh_info;       /* Additional info */
    tak_bertanda32_t sh_addralign;  /* Alignment */
    tak_bertanda32_t sh_entsize;    /* Entry size if section holds table */
} elf32_shdr_t;

/* ELF loader context */
typedef struct {
    berkas_t *berkas;               /* File handle */
    elf32_header_t header;          /* ELF header */
    elf32_phdr_t *phdrs;            /* Program headers */
    tak_bertanda32_t phdr_count;    /* Number of program headers */
    alamat_virtual_t entry;         /* Entry point */
    alamat_virtual_t min_addr;      /* Minimum load address */
    alamat_virtual_t max_addr;      /* Maximum load address */
    ukuran_t total_size;            /* Total memory size needed */
} elf_loader_ctx_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik ELF loader */
static struct {
    tak_bertanda64_t files_loaded;
    tak_bertanda64_t segments_loaded;
    tak_bertanda64_t total_bytes;
    tak_bertanda64_t load_errors;
} elf_stats = {0};

/* Status inisialisasi */
static bool_t elf_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * elf_validasi_header
 * -------------------
 * Validasi ELF header.
 *
 * Parameter:
 *   header - Pointer ke ELF header
 *
 * Return: BENAR jika valid, SALAH jika tidak
 */
static bool_t elf_validasi_header(elf32_header_t *header)
{
    if (header == NULL) {
        return SALAH;
    }

    /* Cek magic number */
    if (header->e_ident[0] != 0x7F ||
        header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' ||
        header->e_ident[3] != 'F') {
        return SALAH;
    }

    /* Cek class (harus 32-bit untuk x86) */
    if (header->e_ident[4] != ELF_CLASS_32) {
        kernel_printf("[ELF] Error: Bukan ELF 32-bit\n");
        return SALAH;
    }

    /* Cek endianness (harus little endian) */
    if (header->e_ident[5] != ELF_DATA_LSB) {
        kernel_printf("[ELF] Error: Bukan little endian\n");
        return SALAH;
    }

    /* Cek version */
    if (header->e_ident[6] != 1) {
        kernel_printf("[ELF] Error: Versi ELF tidak didukung\n");
        return SALAH;
    }

    /* Cek type (harus executable) */
    if (header->e_type != ELF_TYPE_EXEC) {
        kernel_printf("[ELF] Error: Bukan executable (type=%u)\n",
                      header->e_type);
        return SALAH;
    }

    /* Cek machine (harus i386) */
    if (header->e_machine != ELF_MACHINE_386) {
        kernel_printf("[ELF] Error: Machine tidak didukung (%u)\n",
                      header->e_machine);
        return SALAH;
    }

    /* Cek entry point */
    if (header->e_entry == 0) {
        kernel_printf("[ELF] Error: Entry point NULL\n");
        return SALAH;
    }

    return BENAR;
}

/*
 * elf_baca_header
 * ---------------
 * Baca ELF header dari file.
 *
 * Parameter:
 *   berkas - Pointer ke file
 *   header - Pointer untuk menyimpan header
 *
 * Return: Status operasi
 */
static status_t elf_baca_header(berkas_t *berkas, elf32_header_t *header)
{
    ukuran_t bytes_read;

    if (berkas == NULL || header == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Baca header */
    bytes_read = berkas_baca(berkas, header, sizeof(elf32_header_t));

    if (bytes_read != sizeof(elf32_header_t)) {
        return STATUS_FORMAT_INVALID;
    }

    /* Validasi header */
    if (!elf_validasi_header(header)) {
        return STATUS_FORMAT_INVALID;
    }

    return STATUS_BERHASIL;
}

/*
 * elf_baca_phdrs
 * --------------
 * Baca program headers dari file.
 *
 * Parameter:
 *   ctx - Pointer ke loader context
 *
 * Return: Status operasi
 */
static status_t elf_baca_phdrs(elf_loader_ctx_t *ctx)
{
    ukuran_t phdr_size;
    ukuran_t bytes_read;
    tak_bertanda32_t i;

    if (ctx == NULL || ctx->berkas == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Alokasikan buffer untuk program headers */
    phdr_size = ctx->header.e_phnum * ctx->header.e_phentsize;
    ctx->phdrs = (elf32_phdr_t *)kmalloc(phdr_size);

    if (ctx->phdrs == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Seek ke offset program headers */
    berkas_seek(ctx->berkas, ctx->header.e_phoff, SEEK_SET);

    /* Baca program headers */
    bytes_read = berkas_baca(ctx->berkas, ctx->phdrs, phdr_size);

    if (bytes_read != phdr_size) {
        kfree(ctx->phdrs);
        ctx->phdrs = NULL;
        return STATUS_FORMAT_INVALID;
    }

    ctx->phdr_count = ctx->header.e_phnum;

    /* Hitung range alamat yang dibutuhkan */
    ctx->min_addr = 0xFFFFFFFF;
    ctx->max_addr = 0;
    ctx->total_size = 0;

    for (i = 0; i < ctx->phdr_count; i++) {
        elf32_phdr_t *phdr = &ctx->phdrs[i];

        if (phdr->p_type == PT_LOAD) {
            if (phdr->p_vaddr < ctx->min_addr) {
                ctx->min_addr = phdr->p_vaddr;
            }

            if (phdr->p_vaddr + phdr->p_memsz > ctx->max_addr) {
                ctx->max_addr = phdr->p_vaddr + phdr->p_memsz;
            }
        }
    }

    /* Ratakan ke halaman */
    ctx->min_addr = RATAKAN_BAWAH(ctx->min_addr, UKURAN_HALAMAN);
    ctx->max_addr = RATAKAN_ATAS(ctx->max_addr, UKURAN_HALAMAN);
    ctx->total_size = ctx->max_addr - ctx->min_addr;

    return STATUS_BERHASIL;
}

/*
 * elf_load_segment
 * ----------------
 * Load satu segment PT_LOAD.
 *
 * Parameter:
 *   ctx    - Pointer ke loader context
 *   phdr   - Pointer ke program header
 *   vm     - Pointer ke VM descriptor
 *
 * Return: Status operasi
 */
static status_t elf_load_segment(elf_loader_ctx_t *ctx, elf32_phdr_t *phdr,
                                 vm_descriptor_t *vm)
{
    alamat_virtual_t vaddr;
    alamat_virtual_t vaddr_aligned;
    ukuran_t memsz;
    ukuran_t filesz;
    ukuran_t offset;
    tak_bertanda32_t flags;
    tak_bertanda8_t *buffer;
    ukuran_t bytes_read;
    status_t status;

    if (ctx == NULL || phdr == NULL || vm == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Hanya proses PT_LOAD */
    if (phdr->p_type != PT_LOAD) {
        return STATUS_BERHASIL;
    }

    vaddr = phdr->p_vaddr;
    memsz = phdr->p_memsz;
    filesz = phdr->p_filesz;

    /* Hitung flags mapping */
    flags = 0;
    if (phdr->p_flags & PF_R) {
        flags |= VMA_FLAG_READ;
    }
    if (phdr->p_flags & PF_W) {
        flags |= VMA_FLAG_WRITE;
    }
    if (phdr->p_flags & PF_X) {
        flags |= VMA_FLAG_EXEC;
    }

    /* Hitung alamat dan ukuran yang diratakan */
    vaddr_aligned = RATAKAN_BAWAH(vaddr, UKURAN_HALAMAN);
    offset = vaddr - vaddr_aligned;
    memsz = RATAKAN_ATAS(memsz + offset, UKURAN_HALAMAN);

    /* Map region di VM */
    status = vm_map(vm, vaddr_aligned, memsz, flags, VMA_FLAG_NONE);

    if (status != STATUS_BERHASIL) {
        kernel_printf("[ELF] Error: Gagal map segment di 0x%08lX\n",
                      vaddr_aligned);
        return status;
    }

    /* Baca konten segment dari file */
    if (filesz > 0) {
        /* Alokasikan buffer sementara */
        buffer = (tak_bertanda8_t *)kmalloc(filesz);
        if (buffer == NULL) {
            return STATUS_MEMORI_HABIS;
        }

        /* Seek dan baca */
        berkas_seek(ctx->berkas, phdr->p_offset, SEEK_SET);
        bytes_read = berkas_baca(ctx->berkas, buffer, filesz);

        if (bytes_read != filesz) {
            kfree(buffer);
            return STATUS_IO_ERROR;
        }

        /* Copy ke memory */
        kernel_memcpy((void *)vaddr, buffer, filesz);

        kfree(buffer);

        elf_stats.total_bytes += filesz;
    }

    /* Zero-fill sisa memory (BSS) */
    if (memsz > filesz) {
        kernel_memset((void *)(vaddr + filesz), 0, memsz - filesz);
    }

    elf_stats.segments_loaded++;

    return STATUS_BERHASIL;
}

/*
 * elf_load_segments
 * -----------------
 * Load semua segment PT_LOAD.
 *
 * Parameter:
 *   ctx - Pointer ke loader context
 *   vm  - Pointer ke VM descriptor
 *
 * Return: Status operasi
 */
static status_t elf_load_segments(elf_loader_ctx_t *ctx, vm_descriptor_t *vm)
{
    tak_bertanda32_t i;
    status_t status;

    if (ctx == NULL || vm == NULL) {
        return STATUS_PARAM_INVALID;
    }

    for (i = 0; i < ctx->phdr_count; i++) {
        status = elf_load_segment(ctx, &ctx->phdrs[i], vm);

        if (status != STATUS_BERHASIL) {
            return status;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * elf_init
 * --------
 * Inisialisasi subsistem ELF loader.
 *
 * Return: Status operasi
 */
status_t elf_init(void)
{
    if (elf_initialized) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&elf_stats, 0, sizeof(elf_stats));
    elf_initialized = BENAR;

    kernel_printf("[ELF] ELF loader initialized\n");

    return STATUS_BERHASIL;
}

/*
 * elf_load
 * --------
 * Load berkas ELF ke memory proses.
 *
 * Parameter:
 *   path   - Path ke berkas ELF
 *   proses - Pointer ke proses target
 *   info   - Pointer untuk menyimpan informasi binary
 *
 * Return: Status operasi
 */
status_t elf_load(const char *path, proses_t *proses, binary_info_t *info)
{
    elf_loader_ctx_t ctx;
    status_t status;

    if (!elf_initialized) {
        return STATUS_GAGAL;
    }

    if (path == NULL || proses == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Reset context */
    kernel_memset(&ctx, 0, sizeof(ctx));

    /* Buka file */
    ctx.berkas = berkas_buka(path, BERKAS_BACA);
    if (ctx.berkas == NULL) {
        elf_stats.load_errors++;
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Baca header */
    status = elf_baca_header(ctx.berkas, &ctx.header);
    if (status != STATUS_BERHASIL) {
        berkas_tutup(ctx.berkas);
        elf_stats.load_errors++;
        return status;
    }

    /* Baca program headers */
    status = elf_baca_phdrs(&ctx);
    if (status != STATUS_BERHASIL) {
        berkas_tutup(ctx.berkas);
        elf_stats.load_errors++;
        return status;
    }

    /* Load segments */
    status = elf_load_segments(&ctx, proses->vm);
    if (status != STATUS_BERHASIL) {
        kfree(ctx.phdrs);
        berkas_tutup(ctx.berkas);
        elf_stats.load_errors++;
        return status;
    }

    /* Set entry point */
    ctx.entry = ctx.header.e_entry;

    /* Isi info binary */
    if (info != NULL) {
        info->entry = ctx.entry;
        info->phdr = ctx.header.e_phoff;
        info->phnum = ctx.header.e_phnum;
        info->phent = ctx.header.e_phentsize;
        info->type = ctx.header.e_type;
        info->flags = ctx.header.e_flags;
    }

    /* Cleanup */
    kfree(ctx.phdrs);
    berkas_tutup(ctx.berkas);

    elf_stats.files_loaded++;

    return STATUS_BERHASIL;
}

/*
 * elf_load_from_memory
 * --------------------
 * Load ELF dari memory buffer.
 *
 * Parameter:
 *   buffer - Pointer ke buffer
 *   size   - Ukuran buffer
 *   proses - Pointer ke proses target
 *   info   - Pointer untuk informasi binary
 *
 * Return: Status operasi
 */
status_t elf_load_from_memory(const void *buffer, ukuran_t size,
                              proses_t *proses, binary_info_t *info)
{
    elf32_header_t *header;
    elf32_phdr_t *phdrs;
    tak_bertanda32_t i;
    status_t status;

    if (!elf_initialized || buffer == NULL || proses == NULL) {
        return STATUS_PARAM_INVALID;
    }

    if (size < sizeof(elf32_header_t)) {
        return STATUS_FORMAT_INVALID;
    }

    /* Cast ke header */
    header = (elf32_header_t *)buffer;

    /* Validasi header */
    if (!elf_validasi_header(header)) {
        return STATUS_FORMAT_INVALID;
    }

    /* Dapatkan program headers */
    phdrs = (elf32_phdr_t *)((tak_bertanda8_t *)buffer + header->e_phoff);

    /* Load setiap segment */
    for (i = 0; i < header->e_phnum; i++) {
        elf32_phdr_t *phdr = &phdrs[i];

        if (phdr->p_type == PT_LOAD) {
            alamat_virtual_t vaddr = phdr->p_vaddr;
            tak_bertanda32_t flags = 0;

            /* Set flags */
            if (phdr->p_flags & PF_R) flags |= VMA_FLAG_READ;
            if (phdr->p_flags & PF_W) flags |= VMA_FLAG_WRITE;
            if (phdr->p_flags & PF_X) flags |= VMA_FLAG_EXEC;

            /* Map region */
            status = vm_map(proses->vm,
                            RATAKAN_BAWAH(vaddr, UKURAN_HALAMAN),
                            RATAKAN_ATAS(phdr->p_memsz, UKURAN_HALAMAN),
                            flags, VMA_FLAG_NONE);

            if (status != STATUS_BERHASIL) {
                return status;
            }

            /* Copy data */
            if (phdr->p_filesz > 0) {
                kernel_memcpy((void *)vaddr,
                              (tak_bertanda8_t *)buffer + phdr->p_offset,
                              phdr->p_filesz);
            }

            /* Zero BSS */
            if (phdr->p_memsz > phdr->p_filesz) {
                kernel_memset((void *)(vaddr + phdr->p_filesz), 0,
                              phdr->p_memsz - phdr->p_filesz);
            }
        }
    }

    /* Set info */
    if (info != NULL) {
        info->entry = header->e_entry;
        info->phdr = header->e_phoff;
        info->phnum = header->e_phnum;
        info->phent = header->e_phentsize;
    }

    elf_stats.files_loaded++;

    return STATUS_BERHASIL;
}

/*
 * elf_get_stats
 * -------------
 * Dapatkan statistik ELF loader.
 *
 * Parameter:
 *   files     - Pointer untuk jumlah file
 *   segments  - Pointer untuk jumlah segment
 *   bytes     - Pointer untuk total bytes
 *   errors    - Pointer untuk jumlah error
 */
void elf_get_stats(tak_bertanda64_t *files, tak_bertanda64_t *segments,
                   tak_bertanda64_t *bytes, tak_bertanda64_t *errors)
{
    if (files != NULL) {
        *files = elf_stats.files_loaded;
    }

    if (segments != NULL) {
        *segments = elf_stats.segments_loaded;
    }

    if (bytes != NULL) {
        *bytes = elf_stats.total_bytes;
    }

    if (errors != NULL) {
        *errors = elf_stats.load_errors;
    }
}

/*
 * elf_print_stats
 * ---------------
 * Print statistik ELF loader.
 */
void elf_print_stats(void)
{
    kernel_printf("\n[ELF] Statistik ELF Loader:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Files Loaded    : %lu\n", elf_stats.files_loaded);
    kernel_printf("  Segments Loaded : %lu\n", elf_stats.segments_loaded);
    kernel_printf("  Total Bytes     : %lu\n", elf_stats.total_bytes);
    kernel_printf("  Load Errors     : %lu\n", elf_stats.load_errors);
    kernel_printf("========================================\n");
}

/*
 * elf_print_info
 * --------------
 * Print informasi berkas ELF.
 *
 * Parameter:
 *   path - Path ke berkas ELF
 */
void elf_print_info(const char *path)
{
    berkas_t *berkas;
    elf32_header_t header;
    tak_bertanda32_t i;
    elf32_phdr_t phdr;
    const char *type_str;

    if (path == NULL) {
        return;
    }

    berkas = berkas_buka(path, BERKAS_BACA);
    if (berkas == NULL) {
        kernel_printf("[ELF] Tidak dapat membuka: %s\n", path);
        return;
    }

    if (berkas_baca(berkas, &header, sizeof(header)) != sizeof(header)) {
        berkas_tutup(berkas);
        kernel_printf("[ELF] Gagal membaca header\n");
        return;
    }

    /* Validasi magic */
    if (header.e_ident[0] != 0x7F || header.e_ident[1] != 'E' ||
        header.e_ident[2] != 'L' || header.e_ident[3] != 'F') {
        berkas_tutup(berkas);
        kernel_printf("[ELF] Bukan berkas ELF\n");
        return;
    }

    /* Type string */
    switch (header.e_type) {
        case ELF_TYPE_EXEC:
            type_str = "Executable";
            break;
        case ELF_TYPE_DYN:
            type_str = "Shared Object";
            break;
        case ELF_TYPE_REL:
            type_str = "Relocatable";
            break;
        default:
            type_str = "Unknown";
            break;
    }

    kernel_printf("\n[ELF] Informasi Berkas: %s\n", path);
    kernel_printf("========================================\n");
    kernel_printf("  Class:     %s\n",
                  header.e_ident[4] == 2 ? "64-bit" : "32-bit");
    kernel_printf("  Endian:    %s\n",
                  header.e_ident[5] == 2 ? "Big" : "Little");
    kernel_printf("  Type:      %s\n", type_str);
    kernel_printf("  Machine:   %u\n", header.e_machine);
    kernel_printf("  Entry:     0x%08lX\n", header.e_entry);
    kernel_printf("  PHDR:      %u entries at offset %lu\n",
                  header.e_phnum, (tak_bertanda64_t)header.e_phoff);
    kernel_printf("========================================\n");

    /* Print program headers */
    kernel_printf("\n  Program Headers:\n");

    for (i = 0; i < header.e_phnum; i++) {
        berkas_seek(berkas, header.e_phoff + i * header.e_phentsize,
                    SEEK_SET);
        berkas_baca(berkas, &phdr, sizeof(phdr));

        if (phdr.p_type == PT_LOAD) {
            kernel_printf("    LOAD: vaddr=0x%08lX "
                          "filesz=%lu memsz=%lu flags=%c%c%c\n",
                          phdr.p_vaddr,
                          (tak_bertanda64_t)phdr.p_filesz,
                          (tak_bertanda64_t)phdr.p_memsz,
                          (phdr.p_flags & PF_R) ? 'R' : '-',
                          (phdr.p_flags & PF_W) ? 'W' : '-',
                          (phdr.p_flags & PF_X) ? 'X' : '-');
        }
    }

    kernel_printf("========================================\n");

    berkas_tutup(berkas);
}
