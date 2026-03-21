/*
 * PIGURA OS - LINKER SCRIPT ARM64 (AArch64)
 * -----------------------------------------
 * Skrip linker untuk arsitektur ARMv8/ARMv9 64-bit (AArch64).
 *
 * Berkas ini mendefinisikan tata letak memori kernel untuk
 * sistem ARM64 dengan dukungan 4-level paging dan VA 48-bit.
 *
 * Versi: 1.0
 * Tanggal: 2025
 *
 * CATATAN: Alamat virtual kernel di higher half (TTBR1).
 *          Mendukung Virtualization, TrustZone, dan PAN.
 */

OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)

/*
 * Konstanta memori
 */
ALAMAT_VIRTUAL_KERNEL  = 0xFFFFFFFF80000000;  /* -2GB higher half (TTBR1) */
ALAMAT_FISIK_KERNEL     = 0x00080000;          /* 512KB physical load */
UKURAN_HALAMAN          = 0x1000;              /* 4KB page size */
UKURAN_BLOK             = 0x200000;            /* 2MB block */
UKURAN_STACK            = 0x10000;             /* 64KB stack */

/*
 * Definisi memori
 */
MEMORY
{
    RAM (rwx) : ORIGIN = ALAMAT_FISIK_KERNEL, LENGTH = 1024M
}

SECTIONS
{
    /*
     * Kode bootstrap (EL3/EL2)
     */
    . = ALAMAT_FISIK_KERNEL;

    _bootstrap_start = .;

    .bootstrap ALIGN(UKURAN_HALAMAN) :
    {
        *(.bootstrap)
        *(.bootstrap.*)
        *(.boot)
        *(.boot.*)

        /* EL3 Secure Monitor */
        *(.el3_code)
        *(.el3_code.*)

        /* EL2 Hypervisor */
        *(.el2_code)
        *(.el2_code.*)

        /* Exception vectors */
        *(.vectors)
        *(.vectors.*)
    } > RAM

    _bootstrap_end = .;

    /*
     * Kernel utama di higher half (TTBR1 region)
     */
    . = ALAMAT_VIRTUAL_KERNEL + _bootstrap_end;

    _kernel_start = .;

    /*
     * Exception vectors (must be 2KB aligned on AArch64)
     */
    . = ALIGN(0x800);
    _exception_vectors_start = .;
    .exception_vectors ALIGN(0x800) :
    {
        *(.exception_vectors)
        *(.exception_vectors.*)

        /* Current EL with SP0 */
        . = ALIGN(0x80);
        *(.vec_curr_el_sp0)
        /* Current EL with SPx */
        . = ALIGN(0x80);
        *(.vec_curr_el_spx)
        /* Lower EL using AArch64 */
        . = ALIGN(0x80);
        *(.vec_lower_el_aarch64)
        /* Lower EL using AArch32 */
        . = ALIGN(0x80);
        *(.vec_lower_el_aarch32)
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

        /* Hot/cold function sections */
        *(.text.hot)
        *(.text.unlikely)
        *(.text.startup)
        *(.text.exit)

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

        /* String literals */
        *(.rodata.str1.1)
        *(.rodata.str1.8)
        *(.rodata.cst4)
        *(.rodata.cst8)
        *(.rodata.cst16)

        /* Exception handling */
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.gcc_except_table)
        *(.gcc_except_table.*)

        /* AArch64 specific unwind info */
        . = ALIGN(8);
        PROVIDE(__eh_frame_start = .);
        KEEP(*(.eh_frame))
        PROVIDE(__eh_frame_end = .);

        _rodata_end = .;
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

        /* GOT */
        *(.got)
        *(.got.*)

        /* PLT */
        *(.plt)
        *(.plt.*)

        /* Constructors */
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);

        PROVIDE(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE(__fini_array_end = .);

        PROVIDE(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE(__preinit_array_end = .);

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
     * Page tables (4-level on AArch64)
     */
    .page_tables ALIGN(UKURAN_HALAMAN) :
    {
        _page_tables_start = .;

        /* Level 0 Table (PGD) - 512 entries, 64-bit each = 4KB */
        . = ALIGN(UKURAN_HALAMAN);
        _pgd_start = .;
        *(.pgd)
        *(.pgd.*)
        . = . + 0x1000;
        _pgd_end = .;

        /* Level 1 Table (PUD) */
        . = ALIGN(UKURAN_HALAMAN);
        _pud_start = .;
        *(.pud)
        *(.pud.*)
        _pud_end = .;

        /* Level 2 Table (PMD) */
        . = ALIGN(UKURAN_HALAMAN);
        _pmd_start = .;
        *(.pmd)
        *(.pmd.*)
        _pmd_end = .;

        /* Level 3 Table (PTE) */
        . = ALIGN(UKURAN_HALAMAN);
        _pte_start = .;
        *(.page_tables)
        *(.page_tables.*)
        *(.pte)
        *(.pte.*)
        _pte_end = .;

        _page_tables_end = .;
    } > RAM

    /*
     * Stacks for each exception level
     */

    /* EL0 (User) stack - allocated per-process, not here */

    /* EL1 stack (kernel) */
    .stack_el1 ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_el1_bottom = .;
        . += UKURAN_STACK;
        . = ALIGN(16);
        _stack_el1_top = .;
    } > RAM

    /* EL2 stack (hypervisor) */
    .stack_el2 ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_el2_bottom = .;
        . += 0x4000;
        _stack_el2_top = .;
    } > RAM

    /* EL3 stack (secure monitor) */
    .stack_el3 ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_el3_bottom = .;
        . += 0x4000;
        _stack_el3_top = .;
    } > RAM

    /* IRQ stack */
    .stack_irq ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_irq_bottom = .;
        . += 0x4000;
        _stack_irq_top = .;
    } > RAM

    /* Abort stack */
    .stack_abt ALIGN(UKURAN_HALAMAN) (NOLOAD) :
    {
        _stack_abt_bottom = .;
        . += 0x2000;
        _stack_abt_top = .;
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
        . += 0x400000;  /* 4MB initial heap */
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
        *(.gnu.hash)
        *(.hash)
    }
}

/*
 * Assertions
 */
ASSERT(_kernel_size < 0x20000000, "Kernel terlalu besar (> 512MB)")
ASSERT((_text_start & 0xFFF) == 0, "Section .text tidak selaras halaman")
ASSERT((_exception_vectors_start & 0x7FF) == 0, "Exception vectors tidak selaras 2KB")
ASSERT((_stack_el1_top & 0xF) == 0, "Stack tidak selaras 16-byte")
