# =============================================================================
# PIGURA OS - PAGING SETUP x86_64
# =============================================================================
#
# Berkas ini berisi kode assembly untuk setup paging x86_64.
# Paging setup dilakukan sebelum memasuki long mode.
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
.set CR4_PSE,             0x00000010    # Page Size Extension
.set CR4_PAE,             0x00000020    # Physical Address Extension
.set CR4_PGE,             0x00000080    # Page Global Enable

# MSR
.set MSR_EFER,            0xC0000080    # Extended Feature Enable Register
.set EFER_LME,            0x00000100    # Long Mode Enable
.set EFER_NXE,            0x00000800    # No-Execute Enable

# Page table flags
.set PT_PRESENT,          0x01
.set PT_WRITABLE,         0x02
.set PT_USER,             0x04
.set PT_HUGE,             0x80

# Alamat page tables
.set ALAMAT_PML4,         0x100000      # 1 MB
.set ALAMAT_PDPT,         0x101000      # 1 MB + 4 KB
.set ALAMAT_PD,           0x102000      # 1 MB + 8 KB

# Alamat kernel
.set ALAMAT_KERNEL,       0xFFFFFFFF80000000

# =============================================================================
# KODE
# =============================================================================

.section .text
.code32

# -----------------------------------------------------------------------------
# _paging_setup_64
# ----------------
# Setup paging untuk long mode (x86_64).
#
# Fungsi ini membuat struktur paging 4-level:
#   PML4 -> PDPT -> PD -> (2 MB huge pages)
#
# Return:
#   EAX = 1 jika berhasil, 0 jika gagal
# -----------------------------------------------------------------------------
.global _paging_setup_64
.type _paging_setup_64, @function
_paging_setup_64:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %esi
    pushl %edi

    # Langkah 1: Clear page tables (3 halaman)
    movl $ALAMAT_PML4, %edi
    movl $3072, %ecx              # 3 * 1024 dwords = 12 KB
    xorl %eax, %eax
    cld
    rep stosl

    # Langkah 2: Setup PML4[0] -> PDPT (identity mapping)
    movl $ALAMAT_PML4, %edi
    movl $(ALAMAT_PDPT | PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, (%edi)

    # Langkah 3: Setup PML4[511] -> PDPT (higher half mapping)
    movl $(ALAMAT_PDPT | PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, 511 * 8(%edi)

    # Langkah 4: Setup PDPT[0] -> PD (identity mapping)
    movl $ALAMAT_PDPT, %edi
    movl $(ALAMAT_PD | PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, (%edi)

    # Langkah 5: Setup PDPT[510] -> PD (higher half kernel)
    movl $(ALAMAT_PD | PT_PRESENT | PT_WRITABLE), %eax
    movl %eax, 510 * 8(%edi)

    # Langkah 6: Setup PD entries (2 MB huge pages)
    # Identity mapping first 1 GB (512 entries)
    movl $ALAMAT_PD, %edi
    movl $(PT_PRESENT | PT_WRITABLE | PT_HUGE), %eax
    xorl %edx, %edx               # Physical address = 0

    movl $512, %ecx               # 512 entries = 1 GB

.loop_pd:
    movl %eax, (%edi)             # Lower 32 bits
    movl %edx, 4(%edi)            # Upper 32 bits
    addl $(2 << 20), %eax         # Next 2 MB
    adcl $0, %edx                 # Handle carry
    addl $8, %edi                 # Next entry
    loop .loop_pd

    # Langkah 7: Enable PAE di CR4
    movl %cr4, %eax
    orl $CR4_PAE, %eax
    orl $CR4_PGE, %eax
    movl %eax, %cr4

    # Langkah 8: Load PML4 address ke CR3
    movl $ALAMAT_PML4, %eax
    movl %eax, %cr3

    # Langkah 9: Enable long mode di EFER MSR
    movl $MSR_EFER, %ecx
    rdmsr
    orl $EFER_LME, %eax           # Enable long mode
    orl $EFER_NXE, %eax           # Enable NX bit
    wrmsr

    # Langkah 10: Enable paging di CR0
    movl %cr0, %eax
    orl $CR0_PG, %eax
    movl %eax, %cr0

    # Return success
    movl $1, %eax
    jmp .selesai

.selesai:
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    ret

.size _paging_setup_64, . - _paging_setup_64

# -----------------------------------------------------------------------------
# _paging_identity_map
# --------------------
# Melakukan identity mapping untuk range alamat tertentu.
#
# Parameter:
#   EAX = alamat awal
#   ECX = jumlah halaman
# -----------------------------------------------------------------------------
.global _paging_identity_map
.type _paging_identity_map, @function
_paging_identity_map:
    # Untuk saat ini, identity mapping sudah dilakukan di _paging_setup_64
    # Fungsi ini bisa digunakan untuk menambah mapping dinamis
    ret

.size _paging_identity_map, . - _paging_identity_map

# -----------------------------------------------------------------------------
# _paging_map_page
# ----------------
# Memetakan satu halaman.
#
# Parameter:
#   EAX = alamat virtual
#   EDX = alamat fisik
#   ECX = flags
# -----------------------------------------------------------------------------
.global _paging_map_page
.type _paging_map_page, @function
_paging_map_page:
    # TODO: Implementasi mapping dinamis
    ret

.size _paging_map_page, . - _paging_map_page

# =============================================================================
# DATA
# =============================================================================

.section .bss
.align 4096

# Page tables statis (dialokasi di alamat tetap)
.globl _pml4_static
_pml4_static:
    .skip 4096

.globl _pdpt_static
_pdpt_static:
    .skip 4096

.globl _pd_static
_pd_static:
    .skip 4096

.globl _pt_static
_pt_static:
    .skip 4096
