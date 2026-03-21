# =============================================================================
# PIGURA OS - PROTECTED MODE SWITCH (x86)
# =============================================================================
# Berkas ini berisi kode untuk transisi dari Real Mode (16-bit) ke
# Protected Mode (32-bit) pada arsitektur x86.
#
# Fungsi utama:
#   1. Disable interrupt
#   2. Enable A20 line
#   3. Load GDT
#   4. Set protected mode bit di CR0
#   5. Far jump ke kode 32-bit
#
# Arsitektur: x86
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

.set PORT_A20_CMD,        0x64          # Keyboard command port
.set PORT_A20_DATA,       0x60          # Keyboard data port
.set A20_CMD_ENABLE,      0xDD          # A20 enable command
.set CMD_READ_STATUS,     0xD0          # Read output port
.set CMD_WRITE_OUTPUT,    0xD1          # Write output port
.set CMD_ENABLE_A20,      0xDF          # Enable A20 bit

.set CR0_PE,              0x00000001    # Protected mode enable bit
.set CR0_PG,              0x80000000    # Paging enable bit

.set BIOS_A20_CHECK,      0x2401        # BIOS function untuk A20

# =============================================================================
# ENTRY POINT
# =============================================================================

.section .text
.global _masuk_protected_mode
.global _enable_a20
.global _disable_interrupt
.global _load_gdt
.global _load_idt

.code16

# -----------------------------------------------------------------------------
# _masuk_protected_mode
# -----------------------------------------------------------------------------
# Fungsi utama untuk transisi ke protected mode
# Input:  DS:SI = pointer ke struktur GDT
#         ES:DI = pointer ke struktur IDT (opsional)
# Output: Tidak return, lompat ke 32-bit code
# -----------------------------------------------------------------------------
_masuk_protected_mode:
    # Simpan parameter
    pushw %ds
    pushw %si
    pushw %es
    pushw %di

    # Langkah 1: Disable interrupt
    call _disable_interrupt

    # Langkah 2: Enable A20 line
    call _enable_a20
    testb %al, %al
    jz ._gagal_a20

    # Langkah 3: Load GDT
    popw %di                             # Hapus dari stack (tidak perlu)
    popw %es
    popw %si
    popw %ds
    call _load_gdt

    # Langkah 4: Set protected mode bit
    movl %cr0, %eax
    orl $CR0_PE, %eax
    movl %eax, %cr0

    # Langkah 5: Far jump ke 32-bit code segment
    # Reload segment register dengan selector baru
    ljmp $0x08, $._protected_mode_32

._gagal_a20:
    # A20 line gagal di-enable
    movw $._pesan_gagal_a20, %si
    call _tampilkan_string_16
    ret

# =============================================================================
# KODE 32-BIT (Protected Mode)
# =============================================================================

.code32
._protected_mode_32:
    # Setup segment register untuk protected mode
    movw $0x10, %ax                      # Data segment selector
    movw %ax, %ds                        # DS
    movw %ax, %es                        # ES
    movw %ax, %fs                        # FS
    movw %ax, %gs                        # GS
    movw %ax, %ss                        # SS

    # Setup stack
    movl $_stack_protected_top, %esp

    # Clear flags
    pushl $0
    popfl

    # Jump ke entry point kernel
    jmp _entry_kernel_32

# =============================================================================
# FUNGSI-FUNGSI PEMBANTU (16-BIT)
# =============================================================================

.code16

# -----------------------------------------------------------------------------
# _disable_interrupt
# -----------------------------------------------------------------------------
# Disable semua interrupt termasuk NMI
# -----------------------------------------------------------------------------
_disable_interrupt:
    cli                                  # Disable maskable interrupt
    
    # Disable NMI (Non-Maskable Interrupt)
    inb $0x70, %al
    orb $0x80, %al
    outb %al, $0x70
    
    ret

# -----------------------------------------------------------------------------
# _enable_a20
# -----------------------------------------------------------------------------
# Enable A20 line menggunakan beberapa metode
# Output: AL = 1 jika berhasil, 0 jika gagal
# -----------------------------------------------------------------------------
_enable_a20:
    pushw %bx
    pushw %cx
    
    # Metode 1: Coba BIOS
    movw $BIOS_A20_CHECK, %ax
    int $0x15
    jc ._a20_coba_keyboard               # Jika error, coba keyboard
    
    # Verifikasi A20 sudah enable
    call ._cek_a20
    testb %al, %al
    jnz ._a20_berhasil
    
._a20_coba_keyboard:
    # Metode 2: Keyboard controller
    call ._enable_a20_keyboard
    call ._cek_a20
    testb %al, %al
    jnz ._a20_berhasil
    
    # Metode 3: Fast A20 (port 0x92)
    call ._enable_a20_fast
    call ._cek_a20
    testb %al, %al
    jnz ._a20_berhasil
    
    # Semua metode gagal
    xorw %ax, %ax
    jmp ._a20_selesai

._a20_berhasil:
    movb $1, %al

._a20_selesai:
    popw %cx
    popw %bx
    ret

# -----------------------------------------------------------------------------
# ._enable_a20_keyboard
# -----------------------------------------------------------------------------
# Enable A20 via keyboard controller
# -----------------------------------------------------------------------------
._enable_a20_keyboard:
    # Wait untuk keyboard controller siap
    call ._wait_keyboard_cmd
    
    # Disable keyboard
    movb $0xAD, %al
    outb %al, $PORT_A20_CMD
    
    # Wait
    call ._wait_keyboard_cmd
    
    # Read output port
    movb $CMD_READ_STATUS, %al
    outb %al, $PORT_A20_CMD
    
    call ._wait_keyboard_data
    
    # Read current value
    inb $PORT_A20_DATA, %al
    movb %al, %bl
    
    # Wait
    call ._wait_keyboard_cmd
    
    # Write output port dengan A20 bit set
    movb $CMD_WRITE_OUTPUT, %al
    outb %al, $PORT_A20_CMD
    
    call ._wait_keyboard_cmd
    
    # Set A20 bit
    orb $0x02, %bl
    movb %bl, %al
    outb %al, $PORT_A20_DATA
    
    # Wait
    call ._wait_keyboard_cmd
    
    # Re-enable keyboard
    movb $0xAE, %al
    outb %al, $PORT_A20_CMD
    
    call ._wait_keyboard_cmd
    
    ret

._wait_keyboard_cmd:
    pushw %cx
    movw $0xFFFF, %cx
._wait_kb_cmd_loop:
    inb $PORT_A20_CMD, %al
    testb $0x02, %al                     # Input buffer full?
    loopnz ._wait_kb_cmd_loop
    popw %cx
    ret

._wait_keyboard_data:
    pushw %cx
    movw $0xFFFF, %cx
._wait_kb_data_loop:
    inb $PORT_A20_CMD, %al
    testb $0x01, %al                     # Output buffer full?
    loopz ._wait_kb_data_loop
    popw %cx
    ret

# -----------------------------------------------------------------------------
# ._enable_a20_fast
# -----------------------------------------------------------------------------
# Enable A20 via port 0x92 (System Control Port A)
# -----------------------------------------------------------------------------
._enable_a20_fast:
    inb $0x92, %al
    orb $0x02, %al                       # Set A20 bit
    andb $0xFE, %al                      # Clear reset bit (safety)
    outb %al, $0x92
    ret

# -----------------------------------------------------------------------------
# ._cek_a20
# -----------------------------------------------------------------------------
# Verifikasi A20 line sudah enable dengan memory wraparound test
# Output: AL = 1 jika A20 enable, 0 jika tidak
# -----------------------------------------------------------------------------
._cek_a20:
    pushw %ds
    pushw %es
    pushw %di
    pushw %si
    
    # Setup segment untuk test
    xorw %ax, %ax
    movw %ax, %ds                        # DS = 0x0000
    movw $0xFFFF, %ax
    movw %ax, %es                        # ES = 0xFFFF
    
    # Simpan byte asli di 0x7C00
    movw $0x7DFE, %di                    # 0x7C00 + 0x1FE
    movw $0x7E0E, %si                    # 0x7E00 + 0x10E (alias)
    
    # Baca nilai asli
    movb %es:(%di), %al
    pushw %ax                            # Simpan nilai asli
    
    # Tulis nilai berbeda
    notb %al
    movb %al, %es:(%di)
    
    # Baca dari alamat alias
    movb %ds:(%si), %bl
    
    # Kembalikan nilai asli
    popw %ax
    movb %al, %es:(%di)
    
    # Jika nilai sama, A20 tidak enable (wraparound)
    cmpb %al, %bl
    je ._a20_tidak_enable
    
    # A20 enable
    movb $1, %al
    jmp ._cek_a20_selesai
    
._a20_tidak_enable:
    xorb %al, %al

._cek_a20_selesai:
    popw %si
    popw %di
    popw %es
    popw %ds
    ret

# -----------------------------------------------------------------------------
# _load_gdt
# -----------------------------------------------------------------------------
# Load Global Descriptor Table
# Input: DS:SI = pointer ke GDT descriptor
# -----------------------------------------------------------------------------
_load_gdt:
    lgdtl (%si)
    ret

# -----------------------------------------------------------------------------
# _load_idt
# -----------------------------------------------------------------------------
# Load Interrupt Descriptor Table
# Input: DS:SI = pointer ke IDT descriptor
# -----------------------------------------------------------------------------
_load_idt:
    lidtl (%si)
    ret

# -----------------------------------------------------------------------------
# _tampilkan_string_16
# -----------------------------------------------------------------------------
# Tampilkan string null-terminated (16-bit mode)
# Input: SI = pointer ke string
# -----------------------------------------------------------------------------
_tampilkan_string_16:
    pushw %ax
    pushw %bx
    
._tampil_loop:
    lodsb
    testb %al, %al
    jz ._tampil_selesai
    
    movb $0x0E, %ah
    movb $0x00, %bh
    int $0x10
    
    jmp ._tampil_loop
    
._tampil_selesai:
    popw %bx
    popw %ax
    ret

# =============================================================================
# DATA
# =============================================================================

.section .rodata

._pesan_gagal_a20:
    .asciz "ERROR: Gagal mengaktifkan A20 line.\r\n"

# =============================================================================
# BSS
# =============================================================================

.section .bss
.align 16

_stack_protected_bottom:
    .skip 16384                          # 16 KB stack protected mode
_stack_protected_top:
