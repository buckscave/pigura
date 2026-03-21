/*
 * PIGURA OS - LINKER SCRIPT X86 (32-BIT)
 * ---------------------------------------
 * Skrip linker untuk arsitektur Intel x86 32-bit.
 *
 * Berkas ini mendefinisikan tata letak memori kernel untuk
 * sistem 32-bit dengan dukungan paging dan segmentation.
 *
 * Versi: 1.0
 * Tanggal: 2025
 *
 * CATATAN: Memori virtual dimulai dari 1MB dengan identitas mapping.
 */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

/*
 * Konstanta memori
 */
ALAMAT_VIRTUAL_KERNEL  = 0xC0000000;  /* 3GB - higher half kernel */
ALAMAT_FISIK_KERNEL     = 0x00100000;  /* 1MB - physical load address */
UKURAN_HALAMAN          = 0x1000;      /* 4KB page size */
UKURAN_STACK            = 0x8000;      /* 32KB stack */

SECTIONS
{
    /*
     * Kode bootstrap di alamat fisik rendah
     * Bagian ini berjalan sebelum paging diaktifkan
     */
    . = ALAMAT_FISIK_KERNEL;

    _bootstrap_start = .;

    .bootstrap ALIGN(UKURAN_HALAMAN) :
    {
        *(.bootstrap)
        *(.bootstrap.*)
        *(.multiboot)
        *(.multiboot.*)
        *(.boot)
        *(.boot.*)
    }

    _bootstrap_end = .;

    /*
     * Kernel utama di higher half (3GB+)
     * Dapat diakses setelah paging aktif
     */
    . = ALAMAT_VIRTUAL_KERNEL + _bootstrap_end;

    _kernel_start = .;

    /*
     * Section teks - kode executable
     */
    .text ALIGN(UKURAN_HALAMAN) :
    {
        _text_start = .;
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)
        _text_end = .;
    }

    /*
     * Section data read-only
     */
    .rodata ALIGN(UKURAN_HALAMAN) :
    {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)

        /* String literals */
        *(.rodata.str1.1)
        *(.rodata.str1.4)
        *(.rodata.cst4)
        *(.rodata.cst8)

        /* Exception tables */
        *(.eh_frame)
        *(.eh_frame_hdr)

        _rodata_end = .;
    }

    /*
     * Section data init - data terinisialisasi
     */
    .data ALIGN(UKURAN_HALAMAN) :
    {
        _data_start = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)

        /* Small data */
        *(.sdata)
        *(.sdata.*)

        /* Constructors dan destructors */
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);

        PROVIDE(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE(__fini_array_end = .);

        _data_end = .;
    }

    /*
     * Section BSS - data tidak terinisialisasi
     */
    .bss ALIGN(UKURAN_HALAMAN) :
    {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)

        /* Small BSS */
        *(.sbss)
        *(.sbss.*)

        _bss_end = .;
    }

    /*
     * Page tables - harus selaras halaman
     */
    .page_tables ALIGN(UKURAN_HALAMAN) :
    {
        _page_tables_start = .;
        *(.page_tables)
        *(.page_tables.*)
        . = ALIGN(UKURAN_HALAMAN);
        _page_tables_end = .;
    }

    /*
     * Stack kernel
     */
    .stack ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_bottom = .;
        . += UKURAN_STACK;
        _stack_top = .;
    }

    /*
     * Heap kernel awal
     */
    .heap ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _heap_start = .;
        . += 0x100000;  /* 1MB initial heap */
        _heap_end = .;
    }

    /*
     * Akhir kernel
     */
    _kernel_end = .;

    /*
     * Informasi simbol untuk kernel
     */
    _kernel_virtual_start = ALAMAT_VIRTUAL_KERNEL;
    _kernel_virtual_end = _kernel_end;
    _kernel_physical_start = ALAMAT_FISIK_KERNEL;
    _kernel_physical_end = ALAMAT_FISIK_KERNEL + (_kernel_end - _kernel_start);
    _kernel_size = _kernel_end - _kernel_start;

    /*
     * Discard section yang tidak diperlukan
     */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.debug.*)
        *(.stab)
        *(.stabstr)
        *(.ctors)
        *(.dtors)
    }
}

/*
 * Assertion untuk memastikan kernel tidak terlalu besar
 */
ASSERT(_kernel_size < 0x10000000, "Kernel terlalu besar (> 256MB)")

/*
 * Assertion untuk alignment
 */
ASSERT((_text_start & 0xFFF) == 0, "Section .text tidak selaras halaman")
ASSERT((_data_start & 0xFFF) == 0, "Section .data tidak selaras halaman")
ASSERT((_bss_start & 0xFFF) == 0, "Section .bss tidak selaras halaman")
