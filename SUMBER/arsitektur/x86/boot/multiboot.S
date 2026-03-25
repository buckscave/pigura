# =============================================================================
# PIGURA OS - MULTIBOOT HEADER
# =============================================================================
# Header Multiboot untuk kompatibilitas dengan GRUB dan bootloader lain.
# Memungkinkan kernel dimuat langsung oleh bootloader yang mendukung
# spesifikasi Multiboot (seperti GRUB, Syslinux).
#
# Spesifikasi: Multiboot 1.0
# Arsitektur: x86/x86_64
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA MULTIBOOT
# =============================================================================

.set MAGIC_MULTIBOOT,      0x1BADB002    # Magic number Multiboot
.set FLAG_MULTIBOOT,       0x00000007    # Flag: mem, video, align
.set CHECKSUM_MULTIBOOT,   -0x1BADB009   # Checksum (membuat total = 0)

# Mode video
.set MODE_VIDEO,           0             # Mode video linear graphics
.set LEBAR_LAYAR,          1024          # Lebar layar pixel
.set TINGGI_LAYAR,         768           # Tinggi layar pixel
.set KEDALAMAN_WARNA,      32            # Bit per pixel

# =============================================================================
# MULTIBOOT HEADER
# =============================================================================
# Header ini harus berada di dalam 8192 byte pertama kernel
# =============================================================================

.section .multiboot
.global _header_multiboot
.global _magic_multiboot
.global _flags_multiboot

.balign 4                                 # Alignment 4 byte

_header_multiboot:
    .long MAGIC_MULTIBOOT                 # Magic number
    .long FLAG_MULTIBOOT                  # Flags
    .long CHECKSUM_MULTIBOOT              # Checksum

    # Aditional fields jika flag memori diset
    .long 0x00000000                      # header_addr
    .long 0x00100000                      # load_addr (1 MB)
    .long 0x00100000 + 0x100000           # load_end_addr (2 MB)
    .long 0x00100000 + 0x100000           # bss_end_addr
    .long 0x00100020                      # entry_addr

    # Mode video jika flag video diset
    .long MODE_VIDEO                      # mode_type (0 = linear graphics)
    .long LEBAR_LAYAR                     # width
    .long TINGGI_LAYAR                    # height
    .long KEDALAMAN_WARNA                 # depth

# =============================================================================
# STRUKTUR INFORMASI MULTIBOOT
# =============================================================================
# Struktur ini diberikan oleh bootloader ke kernel
# Alamat struktur diteruskan di register EAX
# =============================================================================

.section .data
.global _info_multiboot_template

_info_multiboot_template:
    .long 0                               # flags
    .long 0                               # mem_lower
    .long 0                               # mem_upper
    .long 0                               # boot_device
    .long 0                               # cmdline
    .long 0                               # mods_count
    .long 0                               # mods_addr
    .long 0                               # syms[4]
    .long 0
    .long 0
    .long 0
    .long 0                               # mmap_length
    .long 0                               # mmap_addr
    .long 0                               # drives_length
    .long 0                               # drives_addr
    .long 0                               # config_table
    .long 0                               # boot_loader_name
    .long 0                               # apm_table
    .long 0                               # vbe_control_info
    .long 0                               # vbe_mode_info
    .word 0                               # vbe_mode
    .word 0                               # vbe_interface_seg
    .word 0                               # vbe_interface_off
    .word 0                               # vbe_interface_len
    .long 0                               # framebuffer_addr
    .long 0                               # framebuffer_pitch
    .long 0                               # framebuffer_width
    .long 0                               # framebuffer_height
    .byte 0                               # framebuffer_bpp
    .byte 0                               # framebuffer_type
    .skip 6                               # color_info[6]

# =============================================================================
# KONSTANTA FLAG MULTIBOOT
# =============================================================================

.section .text
.global _multiboot_flag_memori
.global _multiboot_flag_device
.global _multiboot_flag_cmdline
.global _multiboot_flag_modul
.global _multiboot_flag_mmap
.global _multiboot_flag_vbe

.set _multiboot_flag_memori,   0x00000001   # Info memori valid
.set _multiboot_flag_device,   0x00000002   # Boot device valid
.set _multiboot_flag_cmdline,  0x00000004   # Command line valid
.set _multiboot_flag_modul,    0x00000008   # Modules valid
.set _multiboot_flag_mmap,     0x00000040   # Memory map valid
.set _multiboot_flag_vbe,      0x00000200   # VBE info valid

# =============================================================================
# ENTRY POINT KERNEL (dipanggil oleh Multiboot bootloader)
# =============================================================================

.section .text
.global _entry_multiboot
.extern _kernel_entry

.code32
_entry_multiboot:
    # Verifikasi magic number
    cmpl $0x2BADB002, %eax               # Multiboot magic
    jne ._bukan_multiboot

    # Simpan info multiboot
    movl %ebx, _info_multiboot_ptr       # Simpan pointer info

    # Setup stack
    movl $_stack_kernel_top, %esp

    # Clear direction flag
    cld

    # Panggil entry kernel dengan parameter
    pushl _info_multiboot_ptr            # Argumen 2: info multiboot
    pushl $0x2BADB002                    # Argumen 1: magic number
    call _kernel_entry

    # Jika kernel kembali, halt
    cli
    hlt
    jmp .

._bukan_multiboot:
    # Tidak dimuat oleh bootloader multiboot
    # Coba lanjutkan dengan asumsi standar
    movl $0, _info_multiboot_ptr
    movl $_stack_kernel_top, %esp
    cld
    pushl $0
    pushl $0
    call _kernel_entry
    cli
    hlt
    jmp .

# =============================================================================
# DATA
# =============================================================================

.section .bss
.align 16

_stack_kernel_bottom:
    .skip 32768                          # 32 KB stack
_stack_kernel_top:

_info_multiboot_ptr:
    .skip 4                              # Pointer ke info multiboot
