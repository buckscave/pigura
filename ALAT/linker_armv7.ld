/*
 * PIGURA OS - LINKER SCRIPT ARMV7 (32-BIT MODERN)
 * -----------------------------------------------
 * Skrip linker untuk arsitektur ARMv7-A/R 32-bit.
 *
 * Berkas ini mendefinisikan tata letak memori kernel untuk
 * sistem ARMv7 dengan dukungan LPAE, VFP, dan NEON.
 *
 * Versi: 1.0
 * Tanggal: 2025
 *
 * CATATAN: Mendukung Virtualization Extensions dan TrustZone.
 *          Alamat virtual kernel dimulai dari 0xC0000000.
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

/*
 * Konstanta memori
 */
ALAMAT_VIRTUAL_KERNEL  = 0xC0000000;  /* 3GB - higher half kernel */
ALAMAT_FISIK_KERNEL     = 0x40000000;  /* 1GB - typical ARMv7 physical addr */
UKURAN_HALAMAN          = 0x1000;      /* 4KB page size */
UKURAN_HALAMAN_BESAR    = 0x100000;    /* 1MB section */
UKURAN_STACK            = 0x10000;     /* 64KB stack */

/*
 * Definisi memori
 */
MEMORY
{
    RAM (rwx) : ORIGIN = ALAMAT_FISIK_KERNEL, LENGTH = 512M
}

SECTIONS
{
    /*
     * Kode bootstrap
     */
    . = ALAMAT_FISIK_KERNEL;

    _bootstrap_start = .;

    .bootstrap ALIGN(UKURAN_HALAMAN) :
    {
        *(.bootstrap)
        *(.bootstrap.*)
        *(.boot)
        *(.boot.*)

        /* Secure monitor code */
        *(.secure_monitor)
        *(.secure_monitor.*)

        /* Hypervisor code */
        *(.hypervisor)
        *(.hypervisor.*)
    } > RAM

    _bootstrap_end = .;

    /*
     * Kernel utama di higher half
     */
    . = ALAMAT_VIRTUAL_KERNEL + _bootstrap_end;

    _kernel_start = .;

    /*
     * Exception vector table
     * ARMv7 requires 4KB atau 32-byte alignment tergantung mode
     */
    . = ALIGN(0x1000);
    _exception_vectors_start = .;
    .exception_vectors :
    {
        *(.exception_vectors)
        *(.exception_vectors.*)

        /* High vectors (0xFFFF0000) */
        . = ALIGN(0x1000);
        *(.high_vectors)
        *(.high_vectors.*)
    } > RAM
    _exception_vectors_end = .;

    /*
     * Section teks
     */
    .text ALIGN(UKURAN_HALAMAN) :
    {
        _text_start = .;
        *(.text)
        *(.text.*)
        *(.gnu.linkonce.t.*)

        /* ARM/Thumb interworking */
        *(.glue_7)
        *(.glue_7t)
        *(.vfp11_veneer)
        *(.v4_bx)

        /* THUMB code */
        *(.thumb_text)
        *(.thumb_text.*)

        _text_end = .;
    } > RAM

    /*
     * Section read-only
     */
    .rodata ALIGN(UKURAN_HALAMAN) :
    {
        _rodata_start = .;
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
        *(.rodata.str1.4)

        /* ARM attributes */
        *(.ARM.attributes)

        /* Exception tables */
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
     * VFP/NEON save areas
     */
    .vfp_neon ALIGN(8) :
    {
        *(.vfp_save_area)
        *(.neon_save_area)
    } > RAM

    /*
     * Section data
     */
    .data ALIGN(UKURAN_HALAMAN) :
    {
        _data_start = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)

        /* Thread-local storage */
        *(.tdata)
        *(.tdata.*)
        *(.tbss)
        *(.tbss.*)

        /* Small data */
        *(.sdata)
        *(.sdata.*)

        /* Constructors */
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
     * Page tables (LPAE support)
     */
    .page_tables ALIGN(UKURAN_HALAMAN) :
    {
        _page_tables_start = .;

        /* LPAE: Level 0 Table (4096 entries, 64-bit each) */
        . = ALIGN(UKURAN_HALAMAN);
        _l0_table_start = .;
        *(.l0_page_table)
        . = . + 0x2000;  /* 8KB for L0 */
        _l0_table_end = .;

        /* Level 1 Table */
        . = ALIGN(UKURAN_HALAMAN);
        _l1_table_start = .;
        *(.l1_page_table)
        . = . + 0x1000;  /* 4KB for L1 */
        _l1_table_end = .;

        /* Level 2 Tables */
        . = ALIGN(UKURAN_HALAMAN);
        _l2_tables_start = .;
        *(.l2_page_table)
        *(.l2_page_table.*)
        _l2_tables_end = .;

        /* Level 3 Tables */
        . = ALIGN(UKURAN_HALAMAN);
        _l3_tables_start = .;
        *(.page_tables)
        *(.page_tables.*)
        _l3_tables_end = .;

        _page_tables_end = .;
    } > RAM

    /*
     * Stacks for each processor mode
     */
    .stack_fiq ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_fiq_bottom = .;
        . += 0x2000;
        _stack_fiq_top = .;
    } > RAM

    .stack_irq ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_irq_bottom = .;
        . += 0x2000;
        _stack_irq_top = .;
    } > RAM

    .stack_svc ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_svc_bottom = .;
        . += UKURAN_STACK;
        _stack_svc_top = .;
    } > RAM

    .stack_abt ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_abt_bottom = .;
        . += 0x1000;
        _stack_abt_top = .;
    } > RAM

    .stack_und ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_und_bottom = .;
        . += 0x1000;
        _stack_und_top = .;
    } > RAM

    .stack_mon ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_mon_bottom = .;
        . += 0x2000;
        _stack_mon_top = .;
    } > RAM

    /*
     * Per-CPU data
     */
    .percpu ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _percpu_start = .;
        *(.percpu)
        *(.percpu.*)
        . = ALIGN(UKURAN_HALAMAN);
        _percpu_end = .;
    } > RAM

    /*
     * Heap kernel
     */
    .heap ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _heap_start = .;
        . += 0x200000;  /* 2MB initial heap */
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

ASSERT(_kernel_size < 0x10000000, "Kernel terlalu besar (> 256MB)")
