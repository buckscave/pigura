/*
 * PIGURA OS - LINKER SCRIPT ARM (32-BIT LEGACY)
 * ----------------------------------------------
 * Skrip linker untuk arsitektur ARM 32-bit generik.
 *
 * Berkas ini mendefinisikan tata letak memori kernel untuk
 * sistem ARM 32-bit dengan dukungan MMU dan paging.
 *
 * Versi: 1.0
 * Tanggal: 2025
 *
 * CATATAN: Dirancang untuk ARMv5/v6 dengan MMU klasik.
 *          Alamat virtual kernel dimulai dari 0xC0000000.
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

/*
 * Konstanta memori
 */
ALAMAT_VIRTUAL_KERNEL  = 0xC0000000;  /* 3GB - higher half kernel */
ALAMAT_FISIK_KERNEL     = 0x00010000;  /* 64KB - physical load (typical ARM) */
UKURAN_HALAMAN          = 0x1000;      /* 4KB page size */
UKURAN_STACK            = 0x8000;      /* 32KB stack */

/*
 * Definisi memori
 */
MEMORY
{
    RAM (rwx) : ORIGIN = ALAMAT_FISIK_KERNEL, LENGTH = 128M
}

SECTIONS
{
    /*
     * Kode bootstrap - berjalan sebelum MMU aktif
     */
    . = ALAMAT_FISIK_KERNEL;

    _bootstrap_start = .;

    .bootstrap ALIGN(UKURAN_HALAMAN) :
    {
        *(.bootstrap)
        *(.bootstrap.*)
        *(.boot)
        *(.boot.*)
        *(.vectors)
        *(.vectors.*)

        /* Exception vectors ARM */
        . = ALIGN(4);
        *(.text.boot)
    } > RAM

    _bootstrap_end = .;

    /*
     * Kernel utama di higher half
     */
    . = ALAMAT_VIRTUAL_KERNEL + _bootstrap_end;

    _kernel_start = .;

    /*
     * Exception vector table (harus selaras 4KB)
     */
    . = ALIGN(0x1000);
    _exception_vectors_start = .;
    .exception_vectors :
    {
        *(.exception_vectors)
        *(.exception_vectors.*)
    } > RAM
    _exception_vectors_end = .;

    /*
     * Section teks - kode executable
     */
    .text ALIGN(UKURAN_HALAMAN) :
    {
        _text_start = .;
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)

        /* ARM-specific sections */
        *(.glue_7)          /* ARM/Thumb interworking */
        *(.glue_7t)         /* Thumb/ARM interworking */
        *(.vfp11_veneer)    /* VFP11 veneers */
        *(.v4_bx)           /* BX instruction veneers */

        _text_end = .;
    } > RAM

    /*
     * Section data read-only
     */
    .rodata ALIGN(UKURAN_HALAMAN) :
    {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)

        /* ARM-specific rodata */
        *(.rodata.str1.4)   /* String literals 4-byte aligned */

        /* Exception unwinding */
        *(.ARM.extab)
        *(.ARM.extab.*)
        *(.gnu.linkonce.armextab.*)

        _rodata_end = .;
    } > RAM

    /*
     * ARM exception index
     */
    .ARM.exidx ALIGN(8) :
    {
        PROVIDE(__exidx_start = .);
        *(.ARM.exidx)
        *(.ARM.exidx.*)
        *(.gnu.linkonce.armexidx.*)
        PROVIDE(__exidx_end = .);
    } > RAM

    /*
     * Section data init
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

        /* Constructors/destructors */
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);

        PROVIDE(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE(__fini_array_end = .);

        _data_end = .;
    } > RAM

    /*
     * Section BSS
     */
    .bss ALIGN(UKURAN_HALAMAN) :
    {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(.sbss)
        *(.sbss.*)
        _bss_end = .;
    } > RAM

    /*
     * Page tables
     */
    .page_tables ALIGN(UKURAN_HALAMAN) :
    {
        _page_tables_start = .;

        /* First-level page table (Coarse Page Table) */
        . = ALIGN(UKURAN_HALAMAN);
        _l1_table_start = .;
        *(.l1_page_table)
        . = . + 0x4000;  /* 16KB for L1 table (4096 entries) */
        _l1_table_end = .;

        /* Second-level page tables */
        . = ALIGN(UKURAN_HALAMAN);
        _l2_tables_start = .;
        *(.page_tables)
        *(.page_tables.*)
        _l2_tables_end = .;

        _page_tables_end = .;
    } > RAM

    /*
     * Stack (Mode IRQ)
     */
    .stack_irq ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_irq_bottom = .;
        . += 0x1000;
        _stack_irq_top = .;
    } > RAM

    /*
     * Stack (Mode FIQ)
     */
    .stack_fiq ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_fiq_bottom = .;
        . += 0x1000;
        _stack_fiq_top = .;
    } > RAM

    /*
     * Stack (Mode Supervisor - kernel)
     */
    .stack_svc ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_svc_bottom = .;
        . += UKURAN_STACK;
        _stack_svc_top = .;
    } > RAM

    /*
     * Stack (Mode Abort)
     */
    .stack_abt ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_abt_bottom = .;
        . += 0x1000;
        _stack_abt_top = .;
    } > RAM

    /*
     * Stack (Mode Undefined)
     */
    .stack_und ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_und_bottom = .;
        . += 0x1000;
        _stack_und_top = .;
    } > RAM

    /*
     * Heap kernel
     */
    .heap ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _heap_start = .;
        . += 0x100000;  /* 1MB initial heap */
        _heap_end = .;
    } > RAM

    /*
     * Akhir kernel
     */
    _kernel_end = .;

    /*
     * Informasi simbol
     */
    _kernel_virtual_start = ALAMAT_VIRTUAL_KERNEL;
    _kernel_virtual_end = _kernel_end;
    _kernel_physical_start = ALAMAT_FISIK_KERNEL;
    _kernel_physical_end = ALAMAT_FISIK_KERNEL + (_kernel_end - _kernel_start);
    _kernel_size = _kernel_end - _kernel_start;

    /*
     * Discard
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
 * Assertions
 */
ASSERT(_kernel_size < 0x8000000, "Kernel terlalu besar (> 128MB)")
ASSERT((_text_start & 0xFFF) == 0, "Section .text tidak selaras halaman")
ASSERT((_exception_vectors_start & 0xFFF) == 0, "Exception vectors tidak selaras 4KB")
