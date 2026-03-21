# =============================================================================
# PIGURA OS - INTERRUPT SERVICE ROUTINES (ISR) x86
# =============================================================================
#
# Berkas ini berisi handler untuk CPU exceptions dan interrupt gates.
# Setiap ISR menyimpan context, memanggil handler C, dan restore context.
#
# Arsitektur: x86 (32-bit protected mode)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

.set UKURAN_INTERRUPT_FRAME,  72        # Ukuran struct int_frame

# =============================================================================
# MAKRO
# =============================================================================

# -----------------------------------------------------------------------------
# MAKRO_ISR_TANPA_ERROR
# ----------------------
# Membuat ISR untuk exception yang tidak push error code.
# Parameter: vektor - nomor vektor interrupt
# -----------------------------------------------------------------------------
.macro MAKRO_ISR_TANPA_ERROR vektor
.global _isr_\vektor
_isr_\vektor:
    # Push dummy error code untuk konsistensi
    pushl $0
    
    # Push nomor interrupt
    pushl $\vektor
    
    # Jump ke handler umum
    jmp _isr_handler_umum
.endm

# -----------------------------------------------------------------------------
# MAKRO_ISR_DENGAN_ERROR
# -----------------------
# Membuat ISR untuk exception yang push error code.
# Parameter: vektor - nomor vektor interrupt
# -----------------------------------------------------------------------------
.macro MAKRO_ISR_DENGAN_ERROR vektor
.global _isr_\vektor
_isr_\vektor:
    # Error code sudah di-push oleh CPU
    
    # Push nomor interrupt
    pushl $\vektor
    
    # Jump ke handler umum
    jmp _isr_handler_umum
.endm

# =============================================================================
# KODE
# =============================================================================

.section .text
.code32

# -----------------------------------------------------------------------------
# Exception handlers (ISR 0-31)
# -----------------------------------------------------------------------------

# ISR 0: Divide Error (#DE)
MAKRO_ISR_TANPA_ERROR 0

# ISR 1: Debug Exception (#DB)
MAKRO_ISR_TANPA_ERROR 1

# ISR 2: NMI Interrupt
MAKRO_ISR_TANPA_ERROR 2

# ISR 3: Breakpoint (#BP)
MAKRO_ISR_TANPA_ERROR 3

# ISR 4: Overflow (#OF)
MAKRO_ISR_TANPA_ERROR 4

# ISR 5: BOUND Range Exceeded (#BR)
MAKRO_ISR_TANPA_ERROR 5

# ISR 6: Invalid Opcode (#UD)
MAKRO_ISR_TANPA_ERROR 6

# ISR 7: Device Not Available (#NM)
MAKRO_ISR_TANPA_ERROR 7

# ISR 8: Double Fault (#DF) - dengan error code
MAKRO_ISR_DENGAN_ERROR 8

# ISR 9: Coprocessor Segment Overrun
MAKRO_ISR_TANPA_ERROR 9

# ISR 10: Invalid TSS (#TS) - dengan error code
MAKRO_ISR_DENGAN_ERROR 10

# ISR 11: Segment Not Present (#NP) - dengan error code
MAKRO_ISR_DENGAN_ERROR 11

# ISR 12: Stack-Segment Fault (#SS) - dengan error code
MAKRO_ISR_DENGAN_ERROR 12

# ISR 13: General Protection (#GP) - dengan error code
MAKRO_ISR_DENGAN_ERROR 13

# ISR 14: Page Fault (#PF) - dengan error code
MAKRO_ISR_DENGAN_ERROR 14

# ISR 15: Reserved
MAKRO_ISR_TANPA_ERROR 15

# ISR 16: x87 FPU Error (#MF)
MAKRO_ISR_TANPA_ERROR 16

# ISR 17: Alignment Check (#AC) - dengan error code
MAKRO_ISR_DENGAN_ERROR 17

# ISR 18: Machine Check (#MC)
MAKRO_ISR_TANPA_ERROR 18

# ISR 19: SIMD Exception (#XF)
MAKRO_ISR_TANPA_ERROR 19

# ISR 20-31: Reserved
MAKRO_ISR_TANPA_ERROR 20
MAKRO_ISR_TANPA_ERROR 21
MAKRO_ISR_TANPA_ERROR 22
MAKRO_ISR_TANPA_ERROR 23
MAKRO_ISR_TANPA_ERROR 24
MAKRO_ISR_TANPA_ERROR 25
MAKRO_ISR_TANPA_ERROR 26
MAKRO_ISR_TANPA_ERROR 27
MAKRO_ISR_TANPA_ERROR 28
MAKRO_ISR_TANPA_ERROR 29
MAKRO_ISR_TANPA_ERROR 30
MAKRO_ISR_TANPA_ERROR 31

# -----------------------------------------------------------------------------
# IRQ handlers (ISR 32-47)
# -----------------------------------------------------------------------------

# IRQ 0-15
MAKRO_ISR_TANPA_ERROR 32
MAKRO_ISR_TANPA_ERROR 33
MAKRO_ISR_TANPA_ERROR 34
MAKRO_ISR_TANPA_ERROR 35
MAKRO_ISR_TANPA_ERROR 36
MAKRO_ISR_TANPA_ERROR 37
MAKRO_ISR_TANPA_ERROR 38
MAKRO_ISR_TANPA_ERROR 39
MAKRO_ISR_TANPA_ERROR 40
MAKRO_ISR_TANPA_ERROR 41
MAKRO_ISR_TANPA_ERROR 42
MAKRO_ISR_TANPA_ERROR 43
MAKRO_ISR_TANPA_ERROR 44
MAKRO_ISR_TANPA_ERROR 45
MAKRO_ISR_TANPA_ERROR 46
MAKRO_ISR_TANPA_ERROR 47

# -----------------------------------------------------------------------------
# Syscall handler (ISR 128 / 0x80)
# -----------------------------------------------------------------------------

.global _isr_128
_isr_128:
    pushl $0                            # Dummy error code
    pushl $128                          # Syscall number
    jmp _isr_handler_umum

# -----------------------------------------------------------------------------
# _isr_handler_umum
# -----------------
# Handler umum untuk semua ISR.
# Menyimpan context, memanggil handler C, dan restore context.
# -----------------------------------------------------------------------------
.global _isr_handler_umum
_isr_handler_umum:
    # Simpan semua register general-purpose
    pushal                              # EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    
    # Simpan segment register
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    
    # Load kernel data segment
    movw $0x10, %ax                     # Kernel data selector
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    # Setup stack frame untuk handler C
    # Stack sekarang:
    #   [GS] [FS] [ES] [DS] [EDI] [ESI] [EBP] [ESP kolon] [EBX] [EDX] [ECX] [EAX]
    #   [ERR_CODE] [INT_NO] [EIP] [CS] [EFLAGS] [ESP] [SS]
    
    # Push pointer ke stack frame
    pushl %esp                          # Argumen: pointer ke frame
    
    # Panggil handler C
    call _handler_interrupt_c
    
    # Bersihkan argumen
    addl $4, %esp
    
    # Restore segment register
    popl %gs
    popl %fs
    popl %es
    popl %ds
    
    # Restore general-purpose registers
    popal
    
    # Hapus error code dan interrupt number
    addl $8, %esp
    
    # Return dari interrupt
    iret

# =============================================================================
# FUNGSI-FUNGSI KHUSUS
# =============================================================================

# -----------------------------------------------------------------------------
# _isr_page_fault_khusus
# -----------------------
# Handler khusus untuk Page Fault dengan informasi tambahan.
# -----------------------------------------------------------------------------
.global _isr_page_fault_khusus
_isr_page_fault_khusus:
    # Page fault sudah push error code
    pushl $14                           # Page fault vector
    
    # Simpan CR2 (alamat yang menyebabkan page fault)
    movl %cr2, %eax
    pushl %eax                          # Simpan untuk handler
    
    # Simpan context seperti biasa
    pushal
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    
    # Load kernel data segment
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    # Panggil handler page fault
    pushl %esp
    call _handler_page_fault_c
    addl $4, %esp
    
    # Hapus CR2 yang disimpan
    addl $4, %esp
    
    # Restore
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal
    addl $8, %esp
    iret

# -----------------------------------------------------------------------------
# _isr_double_fault_khusus
# -------------------------
# Handler khusus untuk Double Fault (non-recoverable).
# -----------------------------------------------------------------------------
.global _isr_double_fault_khusus
_isr_double_fault_khusus:
    # Double fault push error code
    pushl $8                            # Double fault vector
    
    # Simpan context
    pushal
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    
    # Load kernel data segment
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    
    # Double fault fatal - halt sistem
    call _handler_double_fault_c
    
    # Tidak return dari double fault
    cli
    hlt
    jmp .

# -----------------------------------------------------------------------------
# _interrupt_enable / _interrupt_disable
# ---------------------------------------
# Fungsi untuk enable/disable interrupt.
# -----------------------------------------------------------------------------
.global _interrupt_enable
_interrupt_enable:
    sti
    ret

.global _interrupt_disable
_interrupt_disable:
    cli
    ret

# -----------------------------------------------------------------------------
# _interrupt_simpan / _interrupt_kembalikan
# ------------------------------------------
# Fungsi untuk menyimpan dan mengembalikan status interrupt.
# -----------------------------------------------------------------------------
.global _interrupt_simpan
_interrupt_simpan:
    pushfl
    popl %eax
    andl $0x200, %eax                   # IF flag
    shr $9, %eax                        # Shift ke bit 0
    ret

.global _interrupt_kembalikan
_interrupt_kembalikan:
    pushl %eax
    popfl
    ret

# =============================================================================
# DATA
# =============================================================================

.section .rodata

# Tabel alamat ISR untuk inisialisasi IDT
.global _tabel_alamat_isr
_tabel_alamat_isr:
    .long _isr_0
    .long _isr_1
    .long _isr_2
    .long _isr_3
    .long _isr_4
    .long _isr_5
    .long _isr_6
    .long _isr_7
    .long _isr_8
    .long _isr_9
    .long _isr_10
    .long _isr_11
    .long _isr_12
    .long _isr_13
    .long _isr_14
    .long _isr_15
    .long _isr_16
    .long _isr_17
    .long _isr_18
    .long _isr_19
    .long _isr_20
    .long _isr_21
    .long _isr_22
    .long _isr_23
    .long _isr_24
    .long _isr_25
    .long _isr_26
    .long _isr_27
    .long _isr_28
    .long _isr_29
    .long _isr_30
    .long _isr_31
    .long _isr_32
    .long _isr_33
    .long _isr_34
    .long _isr_35
    .long _isr_36
    .long _isr_37
    .long _isr_38
    .long _isr_39
    .long _isr_40
    .long _isr_41
    .long _isr_42
    .long _isr_43
    .long _isr_44
    .long _isr_45
    .long _isr_46
    .long _isr_47
