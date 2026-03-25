# =============================================================================
# PIGURA OS - BOOT SECTOR x86_64
# =============================================================================
#
# Berkas ini berisi kode boot sector untuk arsitektur x86_64.
# Boot sector ini memulai proses boot dari real mode, masuk ke
# protected mode, dan kemudian ke long mode.
#
# Arsitektur: x86_64
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

.set BOOTSEG,        0x07C0
.set LOADSEG,        0x1000
.set LOADADDR,       0x10000

.set CR0_PE,         0x00000001
.set CR0_PG,         0x80000000
.set CR4_PAE,        0x00000020

# =============================================================================
# BOOT SECTOR (16-bit Real Mode)
# =============================================================================

.section .boot, "ax"
.code16
.global _start

_start:
    # Setup segment registers
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7C00, %sp

    # Tampilkan pesan boot
    movw $boot_msg, %si
    call print_string

    # Load stage 2 dari disk
    movw $0x0201, %ax               # AH=02h (read), AL=01 (1 sector)
    movw $LOADSEG, %bx              # ES:BX = buffer
    movw $0x0002, %cx               # CH=cylinder, CL=sector (sector 2)
    movw $0x0000, %dx               # DH=head, DL=drive
    int $0x13
    jc disk_error

    # Jump ke stage 2
    ljmp $LOADSEG, $0

# -----------------------------------------------------------------------------
# print_string
# ------------
# Mencetak string null-terminated.
#
# Parameter:
#   SI = pointer ke string
# -----------------------------------------------------------------------------
print_string:
    movb $0x0E, %ah
.loop:
    lodsb
    testb %al, %al
    jz .done
    int $0x10
    jmp .loop
.done:
    ret

# -----------------------------------------------------------------------------
# disk_error
# ----------
# Handler untuk error pembacaan disk.
# -----------------------------------------------------------------------------
disk_error:
    movw $error_msg, %si
    call print_string
.halt:
    hlt
    jmp .halt

# =============================================================================
# DATA
# =============================================================================

boot_msg:
    .ascii "Pigura OS x86_64 Booting...\r\n"
    .byte 0

error_msg:
    .ascii "Disk read error!\r\n"
    .byte 0

# Padding dan signature
.org 510
.word 0xAA55
