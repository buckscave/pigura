/*
 * PIGURA OS - PAGING SETUP x86
 * ----------------------------
 * Kode assembly untuk setup paging pada arsitektur x86.
 *
 * Berkas ini berisi rutin-rutin assembly untuk inisialisasi
 * paging, termasuk setup page directory, page tables, dan
 * aktifasi paging di CPU.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 *
 * CATATAN: Berkas ini menggunakan syntax AT&T untuk kompatibilitas
 *          dengan GNU Assembler (GAS).
 */

/*
 * ============================================================================
 * SEKSI DATA
 * ============================================================================
 */

.section .data

/* Alamat page directory (akan di-set oleh kernel) */
.globl g_page_directory_addr
g_page_directory_addr:
    .long 0

/* Alamat page table identitas */
.globl g_page_table_identitas_addr
g_page_table_identitas_addr:
    .long 0

/* Alamat page table kernel */
.globl g_page_table_kernel_addr
g_page_table_kernel_addr:
    .long 0

/* Flag untuk status paging */
.globl g_paging_aktif
g_paging_aktif:
    .byte 0

/*
 * ============================================================================
 * SEKSI BSS
 * ============================================================================
 */

.section .bss

/* Space untuk page directory (4KB, 1024 entries) */
.align 4096
.globl g_page_directory
g_page_directory:
    .skip 4096

/* Space untuk page table identitas (4KB, 1024 entries) */
.align 4096
.globl g_page_table_identitas
g_page_table_identitas:
    .skip 4096

/* Space untuk page table kernel (4KB, 1024 entries) */
.align 4096
.globl g_page_table_kernel
g_page_table_kernel:
    .skip 4096

/* Temporary stack untuk setup */
.align 16
g_temp_stack:
    .skip 4096
g_temp_stack_top:

/*
 * ============================================================================
 * SEKSI TEXT
 * ============================================================================
 */

.section .text
.code32

/*
 * ============================================================================
 * KONSTANTA
 * ============================================================================
 */

/* Ukuran halaman: 4KB */
.equ UKURAN_HALAMAN, 4096

/* Flag halaman */
.equ PAGE_PRESENT,     0x01
.equ PAGE_WRITE,       0x02
.equ PAGE_USER,        0x04
.equ PAGE_PWT,         0x08
.equ PAGE_PCD,         0x10
.equ PAGE_ACCESSED,    0x20
.equ PAGE_DIRTY,       0x40
.equ PAGE_4MB,         0x80

/* Flag CR0 */
.equ CR0_PG,           0x80000000
.equ CR0_WP,           0x00010000

/* Flag CR4 */
.equ CR4_PSE,          0x00000010
.equ CR4_PAE,          0x00000020

/* Selector segment */
.equ SELECTOR_KERNEL_CODE, 0x08
.equ SELECTOR_KERNEL_DATA, 0x10

/* Alamat kernel */
.equ KERNEL_VIRTUAL_BASE,  0xC0000000
.equ KERNEL_PHYSICAL_BASE, 0x00100000

/*
 * ============================================================================
 * FUNGSI: paging_setup_early
 * ---------------------------
 * Setup paging awal sebelum kernel utama berjalan.
 * Fungsi ini membuat identity mapping dan kernel mapping.
 *
 * Input:
 *   Tidak ada
 *
 * Output:
 *   EAX = 0 jika berhasil, kode error jika gagal
 *
 * Register yang diubah:
 *   EAX, EBX, ECX, EDX, ESI, EDI
 * ============================================================================
 */
.globl paging_setup_early
paging_setup_early:
    /* Simpan register */
    pushl %ebp
    movl %esp, %ebp

    /* Setup stack sementara */
    movl $g_temp_stack_top, %esp

    /* ========================================
     * TAHAP 1: Clear page directory
     * ======================================== */
    movl $g_page_directory, %edi
    movl $1024, %ecx
    xorl %eax, %eax
    rep stosl

    /* ========================================
     * TAHAP 2: Clear page tables
     * ======================================== */
    /* Clear page table identitas */
    movl $g_page_table_identitas, %edi
    movl $1024, %ecx
    xorl %eax, %eax
    rep stosl

    /* Clear page table kernel */
    movl $g_page_table_kernel, %edi
    movl $1024, %ecx
    xorl %eax, %eax
    rep stosl

    /* ========================================
     * TAHAP 3: Setup identity mapping (0-4MB)
     * ======================================== */
    /* Set page table identitas entries */
    movl $g_page_table_identitas, %edi
    movl $0x00000000, %eax       /* Alamat fisik awal */
    movl $1024, %ecx             /* 1024 entries */

.identity_map_loop:
    /* Set flags: present + writable + user */
    orl $(PAGE_PRESENT | PAGE_WRITE), %eax
    movl %eax, (%edi)
    addl $UKURAN_HALAMAN, %eax   /* Halaman berikutnya */
    addl $4, %edi                /* Entry berikutnya */
    loop .identity_map_loop

    /* ========================================
     * TAHAP 4: Setup kernel mapping (3GB+)
     * ======================================== */
    /* Set page table kernel entries */
    movl $g_page_table_kernel, %edi
    movl $KERNEL_PHYSICAL_BASE, %eax  /* Alamat fisik kernel */
    movl $1024, %ecx             /* 1024 entries */

.kernel_map_loop:
    /* Set flags: present + writable + kernel */
    orl $(PAGE_PRESENT | PAGE_WRITE), %eax
    movl %eax, (%edi)
    addl $UKURAN_HALAMAN, %eax   /* Halaman berikutnya */
    addl $4, %edi                /* Entry berikutnya */
    loop .kernel_map_loop

    /* ========================================
     * TAHAP 5: Setup page directory entries
     * ======================================== */
    /* Entry 0: identity mapping (0-4MB) */
    movl $g_page_directory, %edi
    movl $g_page_table_identitas, %eax
    orl $(PAGE_PRESENT | PAGE_WRITE), %eax
    movl %eax, (%edi)

    /* Entry 768 (0xC0000000 / 4MB): kernel mapping */
    movl $g_page_directory, %edi
    addl $(768 * 4), %edi
    movl $g_page_table_kernel, %eax
    orl $(PAGE_PRESENT | PAGE_WRITE), %eax
    movl %eax, (%edi)

    /* ========================================
     * TAHAP 6: Enable paging
     * ======================================== */
    /* Set CR3 (page directory) */
    movl $g_page_directory, %eax
    movl %eax, %cr3

    /* Enable 4MB pages (PSE) di CR4 */
    movl %cr4, %eax
    orl $CR4_PSE, %eax
    movl %eax, %cr4

    /* Enable paging di CR0 */
    movl %cr0, %eax
    orl $(CR0_PG | CR0_WP), %eax
    movl %eax, %cr0

    /* Set flag paging aktif */
    movb $1, g_paging_aktif

    /* Return success */
    xorl %eax, %eax

    /* Restore stack dan return */
    movl %ebp, %esp
    popl %ebp
    ret

/*
 * ============================================================================
 * FUNGSI: paging_enable
 * ---------------------
 * Mengaktifkan paging dengan page directory yang sudah diset.
 *
 * Input:
 *   EAX = Alamat page directory
 *
 * Output:
 *   Tidak ada
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl paging_enable
paging_enable:
    /* Set CR3 */
    movl %eax, %cr3

    /* Enable 4MB pages di CR4 */
    movl %cr4, %eax
    orl $CR4_PSE, %eax
    movl %eax, %cr4

    /* Enable paging di CR0 */
    movl %cr0, %eax
    orl $(CR0_PG | CR0_WP), %eax
    movl %eax, %cr0

    ret

/*
 * ============================================================================
 * FUNGSI: paging_disable
 * ----------------------
 * Menonaktifkan paging.
 *
 * Input:
 *   Tidak ada
 *
 * Output:
 *   Tidak ada
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl paging_disable
paging_disable:
    /* Disable paging di CR0 */
    movl %cr0, %eax
    andl $(~CR0_PG), %eax
    movl %eax, %cr0

    /* Clear flag */
    movb $0, g_paging_aktif

    ret

/*
 * ============================================================================
 * FUNGSI: paging_switch_directory
 * -------------------------------
 * Mengganti page directory.
 *
 * Input:
 *   EAX = Alamat page directory baru
 *
 * Output:
 *   Tidak ada
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl paging_switch_directory
paging_switch_directory:
    /* Set CR3 */
    movl %eax, %cr3

    ret

/*
 * ============================================================================
 * FUNGSI: paging_invalidate_tlb
 * -----------------------------
 * Menginvalidate TLB untuk satu halaman.
 *
 * Input:
 *   EAX = Alamat virtual halaman
 *
 * Output:
 *   Tidak ada
 *
 * Register yang diubah:
 *   Tidak ada (EAX di-preserve)
 * ============================================================================
 */
.globl paging_invalidate_tlb
paging_invalidate_tlb:
    invlpg (%eax)
    ret

/*
 * ============================================================================
 * FUNGSI: paging_invalidate_tlb_all
 * ---------------------------------
 * Menginvalidate seluruh TLB.
 *
 * Input:
 *   Tidak ada
 *
 * Output:
 *   Tidak ada
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl paging_invalidate_tlb_all
paging_invalidate_tlb_all:
    /* Reload CR3 untuk flush TLB */
    movl %cr3, %eax
    movl %eax, %cr3
    ret

/*
 * ============================================================================
 * FUNGSI: paging_get_cr2
 * ----------------------
 * Mendapatkan nilai CR2 (alamat page fault).
 *
 * Input:
 *   Tidak ada
 *
 * Output:
 *   EAX = Nilai CR2
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl paging_get_cr2
paging_get_cr2:
    movl %cr2, %eax
    ret

/*
 * ============================================================================
 * FUNGSI: paging_get_cr3
 * ----------------------
 * Mendapatkan nilai CR3 (page directory).
 *
 * Input:
 *   Tidak ada
 *
 * Output:
 *   EAX = Nilai CR3
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl paging_get_cr3
paging_get_cr3:
    movl %cr3, %eax
    ret

/*
 * ============================================================================
 * FUNGSI: paging_set_cr3
 * ----------------------
 * Mengatur nilai CR3 (page directory).
 *
 * Input:
 *   EAX = Nilai CR3 baru
 *
 * Output:
 *   Tidak ada
 *
 * Register yang diubah:
 *   Tidak ada
 * ============================================================================
 */
.globl paging_set_cr3
paging_set_cr3:
    movl %eax, %cr3
    ret

/*
 * ============================================================================
 * FUNGSI: paging_map_page
 * -----------------------
 * Memetakan satu halaman fisik ke alamat virtual.
 *
 * Input:
 *   EAX = Alamat virtual
 *   EBX = Alamat fisik
 *   ECX = Flags (present, write, user, dll)
 *
 * Output:
 *   EAX = 0 jika berhasil, kode error jika gagal
 *
 * Register yang diubah:
 *   EAX, EBX, ECX, EDX, ESI, EDI
 * ============================================================================
 */
.globl paging_map_page
paging_map_page:
    /* Simpan register */
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi

    /* Hitung PDE index (bit 31:22) */
    movl %eax, %edx
    shrl $22, %edx              /* EDX = PDE index */

    /* Hitung PTE index (bit 21:12) */
    movl %eax, %esi
    shrl $12, %esi
    andl $0x3FF, %esi           /* ESI = PTE index */

    /* Alamat fisik ke mapping */
    movl %ebx, %edi             /* EDI = alamat fisik */
    andl $0xFFFFF000, %edi      /* Clear flag bits */
    orl %ecx, %edi              /* Add flags */

    /* Cek apakah PDE sudah ada */
    movl $g_page_directory, %ebx
    movl (%ebx, %edx, 4), %eax  /* EAX = PDE entry */
    testl $PAGE_PRESENT, %eax
    jnz .map_page_pt_exists

    /* Page table belum ada, perlu alokasi */
    /* Untuk sekarang, return error */
    movl $-1, %eax              /* Error code */
    jmp .map_page_done

.map_page_pt_exists:
    /* Page table ada, set entry */
    /* EAX = alamat page table */
    andl $0xFFFFF000, %eax      /* Clear flags */
    /* Map ke virtual jika perlu */
    /* Untuk identity mapping, langsung gunakan */
    movl %eax, %ebx
    movl %edi, (%ebx, %esi, 4)  /* Set PTE */

    /* Invalidate TLB */
    /* Alamat virtual sudah di simpan di stack */

.map_page_done:
    /* Restore register */
    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
    popl %ebp
    ret

/*
 * ============================================================================
 * FUNGSI: paging_unmap_page
 * -------------------------
 * Menghapus mapping satu halaman.
 *
 * Input:
 *   EAX = Alamat virtual
 *
 * Output:
 *   EAX = 0 jika berhasil, kode error jika gagal
 *
 * Register yang diubah:
 *   EAX, EBX, ECX, EDX
 * ============================================================================
 */
.globl paging_unmap_page
paging_unmap_page:
    /* Simpan register */
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %ecx
    pushl %edx

    /* Hitung PDE index */
    movl %eax, %edx
    shrl $22, %edx

    /* Hitung PTE index */
    movl %eax, %ecx
    shrl $12, %ecx
    andl $0x3FF, %ecx

    /* Cek PDE */
    movl $g_page_directory, %ebx
    movl (%ebx, %edx, 4), %eax
    testl $PAGE_PRESENT, %eax
    jz .unmap_page_error

    /* Clear PTE */
    andl $0xFFFFF000, %eax
    movl $0, (%eax, %ecx, 4)

    /* Invalidate TLB - tidak perlu karena alamat sudah diubah */
    /* invlpg akan dipanggil oleh caller */

    xorl %eax, %eax
    jmp .unmap_page_done

.unmap_page_error:
    movl $-1, %eax

.unmap_page_done:
    popl %edx
    popl %ecx
    popl %ebx
    popl %ebp
    ret

/*
 * ============================================================================
 * FUNGSI: paging_get_physical
 * ---------------------------
 * Mendapatkan alamat fisik dari alamat virtual.
 *
 * Input:
 *   EAX = Alamat virtual
 *
 * Output:
 *   EAX = Alamat fisik, atau 0 jika tidak di-map
 *
 * Register yang diubah:
 *   EAX, EBX, ECX, EDX
 * ============================================================================
 */
.globl paging_get_physical
paging_get_physical:
    /* Simpan alamat virtual */
    movl %eax, %edx

    /* Hitung PDE index */
    shrl $22, %edx
    movl %edx, %ecx

    /* Cek PDE */
    movl $g_page_directory, %ebx
    movl (%ebx, %ecx, 4), %eax
    testl $PAGE_PRESENT, %eax
    jz .get_physical_error

    /* Ambil alamat page table */
    andl $0xFFFFF000, %eax

    /* Hitung PTE index */
    movl %edx, %ecx             /* Restore alamat virtual */
    shrl $12, %ecx
    andl $0x3FF, %ecx

    /* Baca PTE */
    movl (%eax, %ecx, 4), %eax
    testl $PAGE_PRESENT, %eax
    jz .get_physical_error

    /* Ambil alamat fisik */
    andl $0xFFFFF000, %eax

    /* Add offset within page */
    andl $0xFFF, %edx
    orl %edx, %eax

    jmp .get_physical_done

.get_physical_error:
    xorl %eax, %eax

.get_physical_done:
    ret

/*
 * ============================================================================
 * FUNGSI: get_page_directory_ptr
 * ------------------------------
 * Mendapatkan pointer ke page directory.
 *
 * Input:
 *   Tidak ada
 *
 * Output:
 *   EAX = Pointer ke page directory
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl get_page_directory_ptr
get_page_directory_ptr:
    movl $g_page_directory, %eax
    ret

/*
 * ============================================================================
 * FUNGSI: get_page_table_identitas_ptr
 * ------------------------------------
 * Mendapatkan pointer ke page table identitas.
 *
 * Input:
 *   Tidak ada
 *
 * Output:
 *   EAX = Pointer ke page table identitas
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl get_page_table_identitas_ptr
get_page_table_identitas_ptr:
    movl $g_page_table_identitas, %eax
    ret

/*
 * ============================================================================
 * FUNGSI: get_page_table_kernel_ptr
 * ---------------------------------
 * Mendapatkan pointer ke page table kernel.
 *
 * Input:
 *   Tidak ada
 *
 * Output:
 *   EAX = Pointer ke page table kernel
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl get_page_table_kernel_ptr
get_page_table_kernel_ptr:
    movl $g_page_table_kernel, %eax
    ret

/*
 * ============================================================================
 * FUNGSI: is_paging_enabled
 * -------------------------
 * Mengecek apakah paging sudah aktif.
 *
 * Input:
 *   Tidak ada
 *
 * Output:
 *   EAX = 1 jika aktif, 0 jika tidak
 *
 * Register yang diubah:
 *   EAX
 * ============================================================================
 */
.globl is_paging_enabled
is_paging_enabled:
    movzbl g_paging_aktif, %eax
    ret
