/*
 * PIGURA OS - ELF.C
 * ------------------
 * Implementasi loader berkas ELF.
 *
 * Berkas ini berisi fungsi-fungsi untuk memuat dan memproses
 * berkas executable dalam format ELF (Executable and Linkable Format)
 * dengan dukungan ELF32 dan ELF64.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan lengkap
 * - Batas 80 karakter per baris
 *
 * Versi: 3.0
 * Tanggal: 2025
 */

#include "../kernel.h"
#include "proses.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* ELF Magic */
#define ELF_MAGIC               0x464C457F  /* 0x7F 'E' 'L' 'F' */

/* ELF Class */
#define ELF_CLASS_NONE          0
#define ELF_CLASS_32            1
#define ELF_CLASS_64            2

/* ELF Data encoding */
#define ELF_DATA_NONE           0
#define ELF_DATA_LSB            1    /* Little endian */
#define ELF_DATA_MSB            2    /* Big endian */

/* ELF Type */
#define ELF_TYPE_NONE           0
#define ELF_TYPE_REL            1    /* Relocatable */
#define ELF_TYPE_EXEC           2    /* Executable */
#define ELF_TYPE_DYN            3    /* Shared object */
#define ELF_TYPE_CORE           4    /* Core file */

/* ELF Machine */
#define ELF_MACHINE_NONE        0
#define ELF_MACHINE_386         3    /* i386 */
#define ELF_MACHINE_X86_64      62   /* x86_64 */
#define ELF_MACHINE_ARM         40   /* ARM */
#define ELF_MACHINE_AARCH64     183  /* ARM64 */

/* Program header type */
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

/* Section header type */
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

/* Segment flags */
#define SEG_FLAG_EXEC           0x01
#define SEG_FLAG_WRITE          0x02
#define SEG_FLAG_READ           0x04

/*
 * ============================================================================
 * STRUKTUR DATA ELF32 (ELF32 DATA STRUCTURES)
 * ============================================================================
 */

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

/*
 * ============================================================================
 * STRUKTUR DATA ELF64 (ELF64 DATA STRUCTURES)
 * ============================================================================
 */

/* ELF64 Header */
typedef struct {
    tak_bertanda8_t  e_ident[16];   /* ELF identification */
    tak_bertanda16_t e_type;        /* Object file type */
    tak_bertanda16_t e_machine;     /* Machine type */
    tak_bertanda32_t e_version;     /* Object file version */
    tak_bertanda64_t e_entry;       /* Entry point address */
    tak_bertanda64_t e_phoff;       /* Program header offset */
    tak_bertanda64_t e_shoff;       /* Section header offset */
    tak_bertanda32_t e_flags;       /* Processor-specific flags */
    tak_bertanda16_t e_ehsize;      /* ELF header size */
    tak_bertanda16_t e_phentsize;   /* Size of program header entry */
    tak_bertanda16_t e_phnum;       /* Number of program header entries */
    tak_bertanda16_t e_shentsize;   /* Size of section header entry */
    tak_bertanda16_t e_shnum;       /* Number of section header entries */
    tak_bertanda16_t e_shstrndx;    /* Section name string table index */
} elf64_header_t;

/* ELF64 Program header */
typedef struct {
    tak_bertanda32_t p_type;        /* Type of segment */
    tak_bertanda32_t p_flags;       /* Segment flags */
    tak_bertanda64_t p_offset;      /* Offset in file */
    tak_bertanda64_t p_vaddr;       /* Virtual address in memory */
    tak_bertanda64_t p_paddr;       /* Physical address */
    tak_bertanda64_t p_filesz;      /* Size of segment in file */
    tak_bertanda64_t p_memsz;       /* Size of segment in memory */
    tak_bertanda64_t p_align;       /* Alignment of segment */
} elf64_phdr_t;

/* ELF64 Section header */
typedef struct {
    tak_bertanda32_t sh_name;       /* Section name */
    tak_bertanda32_t sh_type;       /* Section type */
    tak_bertanda64_t sh_flags;      /* Section flags */
    tak_bertanda64_t sh_addr;       /* Virtual address */
    tak_bertanda64_t sh_offset;     /* Offset in file */
    tak_bertanda64_t sh_size;       /* Size of section */
    tak_bertanda32_t sh_link;       /* Link to another section */
    tak_bertanda32_t sh_info;       /* Additional info */
    tak_bertanda64_t sh_addralign;  /* Alignment */
    tak_bertanda64_t sh_entsize;    /* Entry size if section holds table */
} elf64_shdr_t;

/*
 * ============================================================================
 * STRUKTUR LOADER CONTEXT (LOADER CONTEXT STRUCTURE)
 * ============================================================================
 */

/* ELF loader context */
typedef struct {
    berkas_t *berkas;               /* File handle */
    tak_bertanda32_t class;         /* ELF class (32 or 64 bit) */
    tak_bertanda32_t machine;       /* Target machine */
    tak_bertanda64_t entry;         /* Entry point */
    alamat_virtual_t phdr;          /* Program header address */
    ukuran_t phnum;                 /* Number of program headers */
    ukuran_t phent;                 /* Program header entry size */
    alamat_virtual_t min_addr;      /* Minimum load address */
    alamat_virtual_t max_addr;      /* Maximum load address */
    ukuran_t total_size;            /* Total memory size needed */
    ukuran_t text_size;             /* Text segment size */
    ukuran_t data_size;             /* Data segment size */
    ukuran_t bss_size;              /* BSS segment size */
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
    tak_bertanda64_t elf32_loaded;
    tak_bertanda64_t elf64_loaded;
} elf_stats = {0, 0, 0, 0, 0, 0};

/* Status inisialisasi */
static bool_t elf_initialized = SALAH;

/* Lock */
static spinlock_t elf_lock;

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
 *   header  - Pointer ke header
 *   class   - Pointer untuk menyimpan ELF class
 *   machine - Pointer untuk menyimpan machine type
 *
 * Return: BENAR jika valid
 */
static bool_t elf_validasi_header(const void *header, tak_bertanda32_t *class,
                                  tak_bertanda32_t *machine)
{
    const tak_bertanda8_t *ident;
    
    if (header == NULL) {
        return SALAH;
    }
    
    ident = (const tak_bertanda8_t *)header;
    
    /* Cek magic number */
    if (ident[0] != 0x7F || ident[1] != 'E' ||
        ident[2] != 'L' || ident[3] != 'F') {
        return SALAH;
    }
    
    /* Cek class */
    if (ident[4] != ELF_CLASS_32 && ident[4] != ELF_CLASS_64) {
        kernel_printf("[ELF] Error: Class tidak didukung (%u)\n",
                      ident[4]);
        return SALAH;
    }
    
    /* Cek endianness (harus little endian) */
    if (ident[5] != ELF_DATA_LSB) {
        kernel_printf("[ELF] Error: Bukan little endian\n");
        return SALAH;
    }
    
    /* Cek version */
    if (ident[6] != 1) {
        kernel_printf("[ELF] Error: Versi ELF tidak didukung\n");
        return SALAH;
    }
    
    /* Return class dan machine */
    if (class != NULL) {
        *class = ident[4];
    }
    
    /* Extract machine dari header */
    if (machine != NULL) {
        if (ident[4] == ELF_CLASS_32) {
            const elf32_header_t *h32 = (const elf32_header_t *)header;
            *machine = h32->e_machine;
        } else {
            const elf64_header_t *h64 = (const elf64_header_t *)header;
            *machine = h64->e_machine;
        }
    }
    
    return BENAR;
}

/*
 * elf_validasi_machine
 * --------------------
 * Validasi machine type.
 *
 * Parameter:
 *   machine - Machine type dari ELF header
 *
 * Return: BENAR jika didukung
 */
static bool_t elf_validasi_machine(tak_bertanda32_t machine)
{
#if defined(ARSITEKTUR_X86)
    if (machine == ELF_MACHINE_386) {
        return BENAR;
    }
#elif defined(ARSITEKTUR_X86_64)
    if (machine == ELF_MACHINE_X86_64) {
        return BENAR;
    }
#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)
    if (machine == ELF_MACHINE_ARM) {
        return BENAR;
    }
#elif defined(ARSITEKTUR_ARM64)
    if (machine == ELF_MACHINE_AARCH64) {
        return BENAR;
    }
#endif
    
    kernel_printf("[ELF] Error: Machine tidak didukung (%u)\n", machine);
    return SALAH;
}

/*
 * elf_load_segment_32
 * -------------------
 * Load satu segment dari ELF32.
 *
 * Parameter:
 *   berkas - Pointer ke file
 *   phdr   - Pointer ke program header
 *   vm     - Pointer ke VM descriptor
 *
 * Return: Status operasi
 */
static status_t elf_load_segment_32(berkas_t *berkas, const elf32_phdr_t *phdr,
                                    vm_descriptor_t *vm)
{
    alamat_virtual_t vaddr;
    ukuran_t filesz;
    ukuran_t memsz;
    tak_bertanda32_t flags;
    tak_bertanda8_t *buffer;
    ukuran_t bytes_read;
    status_t status;
    
    if (berkas == NULL || phdr == NULL || vm == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Hanya proses PT_LOAD */
    if (phdr->p_type != PT_LOAD) {
        return STATUS_BERHASIL;
    }
    
    vaddr = (alamat_virtual_t)phdr->p_vaddr;
    filesz = (ukuran_t)phdr->p_filesz;
    memsz = (ukuran_t)phdr->p_memsz;
    
    /* Hitung flags */
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
    
    /* Ratakan alamat ke halaman */
    vaddr = RATAKAN_BAWAH(vaddr, UKURAN_HALAMAN);
    memsz = RATAKAN_ATAS(memsz + (phdr->p_vaddr - vaddr), UKURAN_HALAMAN);
    
    /* Map region */
    status = vm_alloc_at(vm, memsz, flags, vaddr);
    
    if (status != STATUS_BERHASIL) {
        kernel_printf("[ELF] Error: Gagal map segment di 0x%08lX\n",
                      vaddr);
        return status;
    }
    
    /* Baca konten segment dari file */
    if (filesz > 0) {
        buffer = (tak_bertanda8_t *)kmalloc(filesz);
        if (buffer == NULL) {
            return STATUS_MEMORI_HABIS;
        }
        
        berkas_seek(berkas, phdr->p_offset, BERKAS_SEEK_SET);
        bytes_read = berkas_baca(berkas, buffer, filesz);
        
        if (bytes_read != filesz) {
            kfree(buffer);
            return STATUS_IO_ERROR;
        }
        
        /* Copy ke memory */
        kernel_memcpy((void *)(alamat_ptr_t)phdr->p_vaddr, buffer, filesz);
        
        kfree(buffer);
        
        elf_stats.total_bytes += filesz;
    }
    
    /* Zero-fill BSS */
    if (memsz > filesz) {
        kernel_memset((void *)(alamat_ptr_t)(phdr->p_vaddr + filesz),
                      0, memsz - filesz);
    }
    
    elf_stats.segments_loaded++;
    
    return STATUS_BERHASIL;
}

/*
 * elf_load_segment_64
 * -------------------
 * Load satu segment dari ELF64.
 *
 * Parameter:
 *   berkas - Pointer ke file
 *   phdr   - Pointer ke program header
 *   vm     - Pointer ke VM descriptor
 *
 * Return: Status operasi
 */
static status_t elf_load_segment_64(berkas_t *berkas, const elf64_phdr_t *phdr,
                                    vm_descriptor_t *vm)
{
    alamat_virtual_t vaddr;
    ukuran_t filesz;
    ukuran_t memsz;
    tak_bertanda32_t flags;
    tak_bertanda8_t *buffer;
    ukuran_t bytes_read;
    status_t status;
    
    if (berkas == NULL || phdr == NULL || vm == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Hanya proses PT_LOAD */
    if (phdr->p_type != PT_LOAD) {
        return STATUS_BERHASIL;
    }
    
    vaddr = (alamat_virtual_t)phdr->p_vaddr;
    filesz = (ukuran_t)phdr->p_filesz;
    memsz = (ukuran_t)phdr->p_memsz;
    
    /* Hitung flags */
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
    
    /* Ratakan alamat ke halaman */
    vaddr = RATAKAN_BAWAH(vaddr, UKURAN_HALAMAN);
    memsz = RATAKAN_ATAS(memsz + (phdr->p_vaddr - vaddr), UKURAN_HALAMAN);
    
    /* Map region */
    status = vm_alloc_at(vm, memsz, flags, vaddr);
    
    if (status != STATUS_BERHASIL) {
        kernel_printf("[ELF] Error: Gagal map segment di 0x%016llX\n",
                      phdr->p_vaddr);
        return status;
    }
    
    /* Baca konten segment dari file */
    if (filesz > 0) {
        buffer = (tak_bertanda8_t *)kmalloc(filesz);
        if (buffer == NULL) {
            return STATUS_MEMORI_HABIS;
        }
        
        berkas_seek(berkas, phdr->p_offset, BERKAS_SEEK_SET);
        bytes_read = berkas_baca(berkas, buffer, filesz);
        
        if (bytes_read != filesz) {
            kfree(buffer);
            return STATUS_IO_ERROR;
        }
        
        /* Copy ke memory */
        kernel_memcpy((void *)(alamat_ptr_t)phdr->p_vaddr, buffer, filesz);
        
        kfree(buffer);
        
        elf_stats.total_bytes += filesz;
    }
    
    /* Zero-fill BSS */
    if (memsz > filesz) {
        kernel_memset((void *)(alamat_ptr_t)(phdr->p_vaddr + filesz),
                      0, memsz - filesz);
    }
    
    elf_stats.segments_loaded++;
    
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
    spinlock_init(&elf_lock);
    
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
status_t elf_load(const char *path, proses_t *proses, void *info)
{
    berkas_t *berkas;
    tak_bertanda8_t header[64];
    ukuran_t bytes_read;
    tak_bertanda32_t elf_class;
    tak_bertanda32_t machine;
    binary_info_t *bin_info;
    elf_loader_ctx_t ctx;
    status_t status;
    tak_bertanda32_t i;
    
    if (!elf_initialized) {
        return STATUS_GAGAL;
    }
    
    if (path == NULL || proses == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    bin_info = (binary_info_t *)info;
    
    /* Buka file */
    berkas = berkas_buka(path, BERKAS_BACA);
    if (berkas == NULL) {
        elf_stats.load_errors++;
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Baca header */
    bytes_read = berkas_baca(berkas, header, sizeof(header));
    
    if (bytes_read < sizeof(elf32_header_t)) {
        berkas_tutup(berkas);
        elf_stats.load_errors++;
        return STATUS_FORMAT_INVALID;
    }
    
    /* Validasi header */
    if (!elf_validasi_header(header, &elf_class, &machine)) {
        berkas_tutup(berkas);
        elf_stats.load_errors++;
        return STATUS_FORMAT_INVALID;
    }
    
    /* Validasi machine */
    if (!elf_validasi_machine(machine)) {
        berkas_tutup(berkas);
        elf_stats.load_errors++;
        return STATUS_TIDAK_DUKUNG;
    }
    
    /* Init context */
    kernel_memset(&ctx, 0, sizeof(ctx));
    ctx.berkas = berkas;
    ctx.class = elf_class;
    ctx.machine = machine;
    ctx.min_addr = 0xFFFFFFFFUL;
    ctx.max_addr = 0;
    
    /* Load berdasarkan class */
    if (elf_class == ELF_CLASS_32) {
        const elf32_header_t *h32;
        elf32_phdr_t phdr;
        
        h32 = (const elf32_header_t *)header;
        
        /* Validasi type */
        if (h32->e_type != ELF_TYPE_EXEC) {
            berkas_tutup(berkas);
            elf_stats.load_errors++;
            return STATUS_FORMAT_INVALID;
        }
        
        ctx.entry = h32->e_entry;
        ctx.phnum = h32->e_phnum;
        ctx.phent = h32->e_phentsize;
        ctx.phdr = h32->e_phoff;
        
        /* Load setiap segment */
        for (i = 0; i < ctx.phnum; i++) {
            berkas_seek(berkas, ctx.phdr + i * ctx.phent, BERKAS_SEEK_SET);
            bytes_read = berkas_baca(berkas, &phdr, sizeof(phdr));
            
            if (bytes_read != sizeof(phdr)) {
                berkas_tutup(berkas);
                elf_stats.load_errors++;
                return STATUS_IO_ERROR;
            }
            
            if (phdr.p_type == PT_LOAD) {
                status = elf_load_segment_32(berkas, &phdr, proses->vm);
                
                if (status != STATUS_BERHASIL) {
                    berkas_tutup(berkas);
                    elf_stats.load_errors++;
                    return status;
                }
                
                /* Update range */
                if (phdr.p_vaddr < ctx.min_addr) {
                    ctx.min_addr = phdr.p_vaddr;
                }
                if (phdr.p_vaddr + phdr.p_memsz > ctx.max_addr) {
                    ctx.max_addr = phdr.p_vaddr + phdr.p_memsz;
                }
            }
        }
        
        elf_stats.elf32_loaded++;
        
    } else {
        /* ELF64 */
        const elf64_header_t *h64;
        elf64_phdr_t phdr;
        
        h64 = (const elf64_header_t *)header;
        
        /* Validasi type */
        if (h64->e_type != ELF_TYPE_EXEC) {
            berkas_tutup(berkas);
            elf_stats.load_errors++;
            return STATUS_FORMAT_INVALID;
        }
        
        ctx.entry = h64->e_entry;
        ctx.phnum = h64->e_phnum;
        ctx.phent = h64->e_phentsize;
        ctx.phdr = h64->e_phoff;
        
        /* Load setiap segment */
        for (i = 0; i < ctx.phnum; i++) {
            berkas_seek(berkas, ctx.phdr + i * ctx.phent, BERKAS_SEEK_SET);
            bytes_read = berkas_baca(berkas, &phdr, sizeof(phdr));
            
            if (bytes_read != sizeof(phdr)) {
                berkas_tutup(berkas);
                elf_stats.load_errors++;
                return STATUS_IO_ERROR;
            }
            
            if (phdr.p_type == PT_LOAD) {
                status = elf_load_segment_64(berkas, &phdr, proses->vm);
                
                if (status != STATUS_BERHASIL) {
                    berkas_tutup(berkas);
                    elf_stats.load_errors++;
                    return status;
                }
                
                /* Update range */
                if (phdr.p_vaddr < ctx.min_addr) {
                    ctx.min_addr = phdr.p_vaddr;
                }
                if (phdr.p_vaddr + phdr.p_memsz > ctx.max_addr) {
                    ctx.max_addr = phdr.p_vaddr + phdr.p_memsz;
                }
            }
        }
        
        elf_stats.elf64_loaded++;
    }
    
    /* Isi info binary */
    if (bin_info != NULL) {
        bin_info->entry = (alamat_virtual_t)ctx.entry;
        bin_info->phdr = ctx.phdr;
        bin_info->phnum = ctx.phnum;
        bin_info->phent = ctx.phent;
        bin_info->type = elf_class == ELF_CLASS_32 ? 32 : 64;
        bin_info->flags = 0;
    }
    
    /* Update proses */
    proses->entry_point = (alamat_virtual_t)ctx.entry;
    proses->start_code = ctx.min_addr;
    proses->end_code = ctx.max_addr;
    
    berkas_tutup(berkas);
    
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
                              proses_t *proses, void *info)
{
    tak_bertanda32_t elf_class;
    tak_bertanda32_t machine;
    binary_info_t *bin_info;
    tak_bertanda32_t i;
    
    if (!elf_initialized || buffer == NULL || proses == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    if (size < sizeof(elf32_header_t)) {
        return STATUS_FORMAT_INVALID;
    }
    
    /* Validasi header */
    if (!elf_validasi_header(buffer, &elf_class, &machine)) {
        return STATUS_FORMAT_INVALID;
    }
    
    /* Validasi machine */
    if (!elf_validasi_machine(machine)) {
        return STATUS_TIDAK_DUKUNG;
    }
    
    bin_info = (binary_info_t *)info;
    
    /* Load berdasarkan class */
    if (elf_class == ELF_CLASS_32) {
        const elf32_header_t *h32;
        const elf32_phdr_t *phdrs;
        
        h32 = (const elf32_header_t *)buffer;
        
        if (h32->e_type != ELF_TYPE_EXEC) {
            return STATUS_FORMAT_INVALID;
        }
        
        phdrs = (const elf32_phdr_t *)
                ((const tak_bertanda8_t *)buffer + h32->e_phoff);
        
        /* Load setiap segment */
        for (i = 0; i < h32->e_phnum; i++) {
            const elf32_phdr_t *phdr = &phdrs[i];
            
            if (phdr->p_type == PT_LOAD) {
                tak_bertanda32_t flags = 0;
                
                if (phdr->p_flags & PF_R) flags |= VMA_FLAG_READ;
                if (phdr->p_flags & PF_W) flags |= VMA_FLAG_WRITE;
                if (phdr->p_flags & PF_X) flags |= VMA_FLAG_EXEC;
                
                /* Map region */
                vm_alloc_at(proses->vm,
                            RATAKAN_ATAS(phdr->p_memsz, UKURAN_HALAMAN),
                            flags,
                            RATAKAN_BAWAH(phdr->p_vaddr, UKURAN_HALAMAN));
                
                /* Copy data */
                if (phdr->p_filesz > 0) {
                    kernel_memcpy((void *)(alamat_virtual_t)phdr->p_vaddr,
                                  (const tak_bertanda8_t *)buffer +
                                  phdr->p_offset,
                                  phdr->p_filesz);
                }
                
                /* Zero BSS */
                if (phdr->p_memsz > phdr->p_filesz) {
                    kernel_memset((void *)(alamat_virtual_t)
                                  (phdr->p_vaddr + phdr->p_filesz),
                                  0, phdr->p_memsz - phdr->p_filesz);
                }
            }
        }
        
        /* Set info */
        if (bin_info != NULL) {
            bin_info->entry = (alamat_virtual_t)h32->e_entry;
            bin_info->phdr = h32->e_phoff;
            bin_info->phnum = h32->e_phnum;
            bin_info->phent = h32->e_phentsize;
        }
        
        proses->entry_point = (alamat_virtual_t)h32->e_entry;
        
        elf_stats.elf32_loaded++;
        
    } else {
        /* ELF64 */
        const elf64_header_t *h64;
        const elf64_phdr_t *phdrs;
        
        h64 = (const elf64_header_t *)buffer;
        
        if (h64->e_type != ELF_TYPE_EXEC) {
            return STATUS_FORMAT_INVALID;
        }
        
        phdrs = (const elf64_phdr_t *)
                ((const tak_bertanda8_t *)buffer + h64->e_phoff);
        
        /* Load setiap segment */
        for (i = 0; i < h64->e_phnum; i++) {
            const elf64_phdr_t *phdr = &phdrs[i];
            
            if (phdr->p_type == PT_LOAD) {
                tak_bertanda32_t flags = 0;
                
                if (phdr->p_flags & PF_R) flags |= VMA_FLAG_READ;
                if (phdr->p_flags & PF_W) flags |= VMA_FLAG_WRITE;
                if (phdr->p_flags & PF_X) flags |= VMA_FLAG_EXEC;
                
                /* Map region */
                vm_alloc_at(proses->vm,
                            RATAKAN_ATAS(phdr->p_memsz, UKURAN_HALAMAN),
                            flags,
                            RATAKAN_BAWAH(phdr->p_vaddr, UKURAN_HALAMAN));
                
                /* Copy data */
                if (phdr->p_filesz > 0) {
                    kernel_memcpy((void *)(uintptr_t)phdr->p_vaddr,
                                  (const tak_bertanda8_t *)buffer +
                                  phdr->p_offset,
                                  phdr->p_filesz);
                }
                
                /* Zero BSS */
                if (phdr->p_memsz > phdr->p_filesz) {
                    kernel_memset((void *)(uintptr_t)(phdr->p_vaddr + phdr->p_filesz),
                                  0, phdr->p_memsz - phdr->p_filesz);
                }
            }
        }
        
        /* Set info */
        if (bin_info != NULL) {
            bin_info->entry = (alamat_virtual_t)h64->e_entry;
            bin_info->phdr = h64->e_phoff;
            bin_info->phnum = h64->e_phnum;
            bin_info->phent = h64->e_phentsize;
        }
        
        proses->entry_point = (alamat_virtual_t)h64->e_entry;
        
        elf_stats.elf64_loaded++;
    }
    
    elf_stats.files_loaded++;
    
    return STATUS_BERHASIL;
}

/*
 * elf_validasi
 * ------------
 * Validasi ELF header.
 *
 * Parameter:
 *   header - Pointer ke header
 *
 * Return: BENAR jika valid
 */
bool_t elf_validasi(const void *header)
{
    tak_bertanda32_t class;
    tak_bertanda32_t machine;
    
    return elf_validasi_header(header, &class, &machine);
}

/*
 * elf_dapat_class
 * ---------------
 * Dapatkan ELF class.
 *
 * Parameter:
 *   header - Pointer ke header
 *
 * Return: ELF class (32 atau 64)
 */
tak_bertanda32_t elf_dapat_class(const void *header)
{
    const tak_bertanda8_t *ident;
    
    if (header == NULL) {
        return ELF_CLASS_NONE;
    }
    
    ident = (const tak_bertanda8_t *)header;
    
    if (ident[0] != 0x7F || ident[1] != 'E' ||
        ident[2] != 'L' || ident[3] != 'F') {
        return ELF_CLASS_NONE;
    }
    
    return ident[4];
}

/*
 * elf_dapat_machine
 * -----------------
 * Dapatkan target machine.
 *
 * Parameter:
 *   header - Pointer ke header
 *
 * Return: Machine type
 */
tak_bertanda32_t elf_dapat_machine(const void *header)
{
    tak_bertanda32_t class;
    tak_bertanda32_t machine;
    
    if (!elf_validasi_header(header, &class, &machine)) {
        return ELF_MACHINE_NONE;
    }
    
    return machine;
}

/*
 * elf_dapat_statistik
 * -------------------
 * Dapatkan statistik ELF loader.
 *
 * Parameter:
 *   files    - Pointer untuk jumlah file
 *   segments  - Pointer untuk jumlah segment
 *   bytes    - Pointer untuk total bytes
 *   errors   - Pointer untuk jumlah error
 */
void elf_dapat_statistik(tak_bertanda64_t *files,
                         tak_bertanda64_t *segments,
                         tak_bertanda64_t *bytes,
                         tak_bertanda64_t *errors)
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
    kernel_printf("  ELF32 Loaded    : %lu\n", elf_stats.elf32_loaded);
    kernel_printf("  ELF64 Loaded    : %lu\n", elf_stats.elf64_loaded);
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
 *   buffer - Pointer ke buffer berisi data ELF
 */
void elf_print_info(void *buffer)
{
    const tak_bertanda8_t *data = (const tak_bertanda8_t *)buffer;
    tak_bertanda32_t elf_class;
    tak_bertanda32_t machine;
    const char *type_str;
    const char *class_str;
    const char *machine_str;
    tak_bertanda32_t i;
    
    if (data == NULL) {
        return;
    }
    
    /* Validasi magic dari buffer */
    if (!elf_validasi_header(data, &elf_class, &machine)) {
        kernel_printf("[ELF] Bukan berkas ELF\n");
        return;
    }
    
    /* Type string */
    if (elf_class == ELF_CLASS_32) {
        const elf32_header_t *h = (const elf32_header_t *)data;
        switch (h->e_type) {
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
    } else {
        const elf64_header_t *h = (const elf64_header_t *)data;
        switch (h->e_type) {
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
    }
    
    /* Class string */
    class_str = (elf_class == ELF_CLASS_64) ? "64-bit" : "32-bit";
    
    /* Machine string */
    switch (machine) {
        case ELF_MACHINE_386:
            machine_str = "i386";
            break;
        case ELF_MACHINE_X86_64:
            machine_str = "x86_64";
            break;
        case ELF_MACHINE_ARM:
            machine_str = "ARM";
            break;
        case ELF_MACHINE_AARCH64:
            machine_str = "AArch64";
            break;
        default:
            machine_str = "Unknown";
            break;
    }
    
    kernel_printf("\n[ELF] Informasi Berkas ELF\n");
    kernel_printf("========================================\n");
    kernel_printf("  Class:     %s\n", class_str);
    kernel_printf("  Type:      %s\n", type_str);
    kernel_printf("  Machine:   %s\n", machine_str);
    
    if (elf_class == ELF_CLASS_32) {
        const elf32_header_t *h = (const elf32_header_t *)data;
        kernel_printf("  Entry:     0x%08lX\n", h->e_entry);
        kernel_printf("  PHDR:      %u entries at offset %lu\n",
                      h->e_phnum, (tak_bertanda64_t)h->e_phoff);
        
        /* Print program headers */
        kernel_printf("\n  Program Headers:\n");
        
        for (i = 0; i < h->e_phnum; i++) {
            const elf32_phdr_t *phdr = (const elf32_phdr_t *)
                                      (data + h->e_phoff + i * h->e_phentsize);
            
            if (phdr->p_type == PT_LOAD) {
                kernel_printf("    LOAD: vaddr=0x%08lX "
                             "filesz=%lu memsz=%lu flags=%c%c%c\n",
                             phdr->p_vaddr,
                             (tak_bertanda64_t)phdr->p_filesz,
                             (tak_bertanda64_t)phdr->p_memsz,
                             (phdr->p_flags & PF_R) ? 'R' : '-',
                             (phdr->p_flags & PF_W) ? 'W' : '-',
                             (phdr->p_flags & PF_X) ? 'X' : '-');
            }
        }
    } else {
        const elf64_header_t *h = (const elf64_header_t *)data;
        kernel_printf("  Entry:     0x%016llX\n", h->e_entry);
        kernel_printf("  PHDR:      %u entries at offset %llu\n",
                      h->e_phnum, h->e_phoff);
    }
    
    kernel_printf("========================================\n");
}
