# =============================================================================
# PIGURA OS - LONG MODE SWITCH (x86_64)
# =============================================================================
#
# Berkas ini berisi kode untuk transisi dari Protected Mode (32-bit) ke
# Long Mode (64-bit) pada arsitektur x86_64.
#
# Fungsi utama:
#   1. Cek dukungan CPU untuk long mode
#   2. Setup paging untuk long mode (PML4)
#   3. Enable PAE dan long mode di MSR
#   4. Load PML4 address ke CR3
#   5. Enable paging di CR0
#   6. Far jump ke kode 64-bit
#
# Arsitektur: x86_64
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

# CR0 bits
.set CR0_PE,              0x00000001    # Protected mode enable
.set CR0_PG,              0x80000000    # Paging enable

# CR4 bits
.set CR4_PAE,             0x00000020    # Physical Address Extension
.set CR4_PGE,             0x00000080    # Page Global Enable

# MSR
.set MSR_EFER,            0xC0000080    # Extended Feature Enable Register
.set EFER_LME,            0x00000100    # Long Mode Enable
.set EFER_NXE,            0x00000800    # No-Execute Enable

# Page table flags
.set PT_PRESENT,          0x01          # Present
.set PT_WRITABLE,         0x02          # Read/Write
.set PT_USER,             0x04          # User accessible
.set PT_HUGE,             0x80          # Huge page (2 MB)

# Alamat
.set ALAMAT_PML4,         0x100000      # 1 MB - PML4 table
.set ALAMAT_PDPT,         0x101000      # PML4 + 4 KB
.set ALAMAT_PD,           0x102000      # PDPT + 4 KB
.set ALAMAT_KERNEL,       0xFFFFFFFF80000000  # Kernel higher half

# =============================================================================
# KODE 32-BIT (Protected Mode)
# =============================================================================

.section .text
.code32
.global _masuk_long_mode
.global _cek_long_mode
.global _setup_paging_64

# -----------------------------------------------------------------------------
# _masuk_long_mode
# ----------------
# Fungsi utama untuk transisi ke long mode.
# Harus dipanggil dalam protected mode dengan paging disabled.
#
# Return:
#   Tidak return jika berhasil, lompat ke kode 64-bit
#   Return dengan error di EAX jika gagal
# -----------------------------------------------------------------------------
_masuk_long_mode:
    # Simpan return address (untuk error case)
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %esi
    pushl %edi

    # Langkah 1: Cek dukungan CPUID
    call _cek_cpuid
    testl %eax, %eax
    jz ._error_cpuid

    # Langkah 2: Cek dukungan long mode
    call _cek_long_mode
    testl %eax, %eax
    jz ._error_long_mode

    # Langkah 3: Setup paging untuk long mode
    call _setup_paging_64
    testl %eax, %eax
    jz ._error_paging

    # Langkah 4: Enable PAE di CR4
    movl %cr4, %eax
    orl $CR4_PAE, %eax
    orl $CR4_PGE, %eax           # Enable global pages
    movl %eax, %cr4

    # Langkah 5: Load PML4 address ke CR3
    movl $ALAMAT_PML4, %eax
    movl %eax, %cr3

    # Langkah 6: Enable long mode di EFER MSR
    movl $MSR_EFER, %ecx
    rdmsr
    orl $EFER_LME, %eax          # Enable long mode
    orl $EFER_NXE, %eax          # Enable NX bit
    wrmsr

    # Langkah 7: Enable paging di CR0
    movl %cr0, %eax
    orl $CR0_PG, %eax
    movl %eax, %cr0

    # Langkah 8: Load 64-bit GDT
    lgdtl _gdt64_pointer

    # Langkah 9: Far jump ke 64-bit code
    ljmp $0x08, $._long_mode_64

    # Tidak mencapai sini jika berhasil

._error_cpuid:
    movl $1, %eax                # Error code 1: CPUID tidak didukung
    jmp ._selesai

._error_long_mode:
    movl $2, %eax                # Error code 2: Long mode tidak didukung
    jmp ._selesai

._error_paging:
    movl $3, %eax                # Error code 3: Gagal setup paging
    jmp ._selesai

._selesai:
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    ret

# =============================================================================
# KODE 64-BIT (Long Mode)
# =============================================================================

.code64
._long_mode_64:
    # Setup segment register untuk long mode
    movw $0x10, %ax              # Data segment selector
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Setup 64-bit stack
    movq $_stack64_top, %rsp

    # Clear flags
    pushq $0
    popfq

    # Jump ke kernel entry point
    jmp _kernel_entry_64

# =============================================================================
# FUNGSI-FUNGSI PEMBANTU (32-BIT)
# =============================================================================

.code32

# -----------------------------------------------------------------------------
# _cek_cpuid
# ----------
# Cek apakah CPU mendukung instruksi CPUID.
#
# Return:
#   EAX = 1 jika didukung, 0 jika tidak
# -----------------------------------------------------------------------------
_cek_cpuid:
    # Test dengan flip bit 21 di EFLAGS
    pushfl
    popl %eax
    movl %eax, %ecx              # Simpan original

    # Flip ID bit (bit 21)
    xorl $0x00200000, %eax
    pushl %eax
    popfl

    # Check jika bit bisa di-flip
    pushfl
    popl %eax
    pushl %ecx
    popfl

    xorl %ecx, %eax
    shrl $21, %eax
    andl $1, %eax

    ret

# -----------------------------------------------------------------------------
# _cek_long_mode
# --------------
# Cek apakah CPU mendukung long mode (64-bit).
#
# Return:
#   EAX = 1 jika didukung, 0 jika tidak
# -----------------------------------------------------------------------------
_cek_long_mode:
    # Cek extended CPUID functions (function 0x80000000)
    movl $0x80000000, %eax
    cpuid
    cmpl $0x80000001, %eax        # Perlu minimal function 0x80000001
    jb ._no_long_mode

    # Cek long mode bit di extended features
    movl $0x80000001, %eax
    cpuid
    testl $(1 << 29), %edx        # Bit 29 = Long Mode
    jz ._no_long_mode

    movl $1, %eax
    ret

._no_long_mode:
    xorl %eax, %eax
    ret

# -----------------------------------------------------------------------------
# _setup_paging_64
# ----------------
# Setup page tables untuk long mode.
#
# Struktur:
#   PML4[0] -> PDPT
#   PDPT[0] -> PD (identity mapping 0-2GB)
#   PD[n]   -> 2 MB huge pages
#
# Return:
#   EAX = 1 jika berhasil, 0 jika gagal
# -----------------------------------------------------------------------------
_setup_paging_64:
    # Clear PML4, PDPT, PD (3 x 4KB = 12KB)
    movl $ALAMAT_PML4, %edi
    movl $3072, %ecx              # 3 * 1024 dwords = 12 KB
    xorl %eax, %eax
    cld
    rep stosl

    # Setup PML4[0] -> PDPT
    movl $ALAMAT_PML4, %edi
    movl $(ALAMAT_PDPT | PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, (%edi)

    # Setup PML4[511] -> PDPT (higher half mapping)
    movl $(ALAMAT_PDPT | PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, 511 * 8(%edi)

    # Setup PDPT[0] -> PD (identity mapping)
    movl $ALAMAT_PDPT, %edi
    movl $(ALAMAT_PD | PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, (%edi)

    # Setup PDPT[510] -> PD (higher half kernel mapping)
    movl $(ALAMAT_PD | PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, 510 * 8(%edi)

    # Setup PD entries (2 MB huge pages)
    # Identity mapping first 1 GB (512 entries x 2 MB)
    movl $ALAMAT_PD, %edi
    movl $(PT_PRESENT | PT_WRITABLE | PT_HUGE), %eax
    movl $0, %edx                 # Physical address = 0

    movl $512, %ecx               # 512 entries (1 GB)

._pd_loop:
    movl %eax, (%edi)             # Lower 32 bits
    movl %edx, 4(%edi)            # Upper 32 bits
    addl $(2 << 20), %eax         # Next 2 MB
    adcl $0, %edx                 # Handle carry
    addl $8, %edi                 # Next entry
    loop ._pd_loop

    movl $1, %eax                 # Success
    ret

# =============================================================================
# DATA
# =============================================================================

.section .rodata

# -----------------------------------------------------------------------------
# GDT 64-bit
# -----------------------------------------------------------------------------
.align 16
_gdt64:
    # Null descriptor
    .quad 0

    # Code segment (64-bit)
    # Base=0, Limit=0, Access=Present|Ring0|Code|Executable|Readable
    # Flags=64-bit|Granularity 4KB
    .quad 0x00AF9A000000FFFF

    # Data segment (64-bit)
    # Base=0, Limit=0, Access=Present|Ring0|Data|Writable
    .quad 0x00AF92000000FFFF

    # User code segment (Ring 3)
    .quad 0x00AFFA000000FFFF

    # User data segment (Ring 3)
    .quad 0x00AFF2000000FFFF

_gdt64_pointer:
    .word . - _gdt64 - 1          # Limit
    .long _gdt64                  # Base address

# =============================================================================
# BSS
# =============================================================================

.section .bss
.align 16

_stack64_bottom:
    .skip 65536                   # 64 KB stack untuk 64-bit mode
_stack64_top:

# Page tables (akan dialokasi dinamis atau di alamat tetap)
.align 4096
_pml4:
    .skip 4096                    # Page Map Level 4
_pdpt:
    .skip 4096                    # Page Directory Pointer Table
_pd:
    .skip 4096                    # Page Directory
_pt:
    .skip 4096                    # Page Table (opsional, untuk 4KB pages)
