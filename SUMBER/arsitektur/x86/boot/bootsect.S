# =============================================================================
# PIGURA OS - BOOT SECTOR (x86)
# =============================================================================
# Berkas ini merupakan boot sector pertama yang dibaca BIOS dari disk.
# Ukuran tepat 512 byte dengan signature 0xAA55 di akhir.
#
# Fungsi utama:
#   1. Inisialisasi segment register dan stack
#   2. Menampilkan pesan boot
#   3. Memuat stage 2 bootloader dari disk
#   4. Melompat ke stage 2
#
# Arsitektur: x86 (16-bit real mode)
# Versi: 1.0
# =============================================================================

# =============================================================================
# KONSTANTA
# =============================================================================

.set UKURAN_SEKTOR,      512         # Ukuran sektor disk
.set SEKTOR_STAGE2,      2           # Stage2 dimulai dari sektor 2
.set JUMLAH_SEKTOR,      16          # Jumlah sektor stage2 (8 KB)
.set ALAMAT_STAGE2,      0x7E00      # Alamat memori stage2
.set ALAMAT_STACK,       0x7C00      # Stack di bawah boot sector

.set BIOS_VIDEO,         0x10        # Interrupt video BIOS
.set BIOS_DISK,          0x13        # Interrupt disk BIOS

.set WARNA_CYAN,         0x0B        # Warna teks cyan terang
.set WARNA_PUTIH,        0x0F        # Warna teks putih

# =============================================================================
# KODE BOOT SECTOR
# =============================================================================

.code16
.section .text
.global _start
.global _gagal

_start:
    # -------------------------------------------------------------------------
    # Inisialisasi segment register
    # Setelah BIOS melompat ke sini, CS mungkin tidak 0x7C0
    # -------------------------------------------------------------------------
    jmp $0x7C0, $._mulai

._mulai:
    # Setup segment register ke nilai yang konsisten
    cli                             # Disable interrupt sementara
    xorw %ax, %ax                   # AX = 0
    movw %ax, %ds                   # Data segment
    movw %ax, %es                   # Extra segment
    movw %ax, %fs                   # FS segment
    movw %ax, %gs                   # GS segment
    movw %ax, %ss                   # Stack segment
    movw $ALAMAT_STACK, %sp         # Stack pointer
    sti                             # Enable interrupt

    # -------------------------------------------------------------------------
    # Simpan drive boot
    # BIOS menempatkan drive boot di DL
    # -------------------------------------------------------------------------
    movb %dl, _drive_boot           # Simpan untuk penggunaan nanti

    # -------------------------------------------------------------------------
    # Tampilkan pesan boot
    # -------------------------------------------------------------------------
    movw $._pesan_judul, %si
    call _tampilkan_string

    # -------------------------------------------------------------------------
    # Reset disk system
    # -------------------------------------------------------------------------
    xorw %ax, %ax                   # Fungsi 0: Reset disk
    movb _drive_boot, %dl           # Drive boot
    int $BIOS_DISK
    jc _gagal_disk                  # Jump jika error (CF=1)

    # -------------------------------------------------------------------------
    # Muat stage 2 bootloader dari disk
    # -------------------------------------------------------------------------
    movw $._pesan_muat, %si
    call _tampilkan_string

    # Setup parameter untuk membaca disk
    movw $0x0200 | JUMLAH_SEKTOR, %ax   # AH=02 (baca), AL=jumlah sektor
    movw $ALAMAT_STAGE2 >> 4, %es       # ES:BX = buffer tujuan
    xorw %bx, %bx                       # BX = 0
    movw $SEKTOR_STAGE2, %cx            # CH=cylinder, CL=sektor
    movb _drive_boot, %dl               # Drive boot
    int $BIOS_DISK
    jc _gagal_disk                      # Jump jika error

    # Verifikasi bahwa stage2 sudah dimuat
    cmpw $0x7E00, %bx                   # BX harus berubah
    je _gagal_kosong                    # Jika sama, tidak ada data

    # -------------------------------------------------------------------------
    # Tampilkan pesan sukses
    # -------------------------------------------------------------------------
    movw $._pesan_sukses, %si
    call _tampilkan_string

    # -------------------------------------------------------------------------
    # Lompat ke stage 2 bootloader
    # -------------------------------------------------------------------------
    jmp $0x0000, $ALAMAT_STAGE2

# =============================================================================
# FUNGSI-FUNGSI PEMBANTU
# =============================================================================

# -----------------------------------------------------------------------------
# _tampilkan_string
# -----------------------------------------------------------------------------
# Menampilkan string null-terminated di layar menggunakan BIOS teletype
# Input:  SI = alamat string
# Output: String ditampilkan di layar
# Register yang diubah: AX, BX, SI
# -----------------------------------------------------------------------------
_tampilkan_string:
    pusha                           # Simpan semua register
    movb $WARNA_PUTIH, %bl          # Warna teks (untuk mode grafis)

._tampilkan_loop:
    lodsb                           # AL = byte dari DS:SI, SI++
    testb %al, %al                  # Cek null terminator
    jz ._tampilkan_selesai          # Jika null, selesai

    movb $0x0E, %ah                 # AH = 0x0E (teletype output)
    movb $0x00, %bh                 # BH = halaman video 0
    int $BIOS_VIDEO                 # Tampilkan karakter

    jmp ._tampilkan_loop            # Lanjut karakter berikutnya

._tampilkan_selesai:
    popa                            # Kembalikan semua register
    ret

# -----------------------------------------------------------------------------
# _tampilkan_hex
# -----------------------------------------------------------------------------
# Menampilkan nilai byte dalam format heksadesimal
# Input:  AL = nilai byte
# Output: 2 karakter heksadesimal di layar
# -----------------------------------------------------------------------------
_tampilkan_hex:
    pusha
    movb %al, %ah                   # Simpan nilai asli
    
    # Karakter pertama (nibble atas)
    shrb $4, %al                    # Geser 4 bit kanan
    call ._tampilkan_nibble
    
    # Karakter kedua (nibble bawah)
    movb %ah, %al
    andb $0x0F, %al                 # Mask nibble bawah
    call ._tampilkan_nibble
    
    popa
    ret

._tampilkan_nibble:
    cmpb $10, %al                   # Cek apakah >= 10
    jb ._digit                      # Jika < 10, angka
    addb $('A' - '0' - 10), %al     # Konversi ke A-F
._digit:
    addb $('0'), %al                # Konversi ke ASCII
    movb $0x0E, %ah                 # Teletype output
    movb $0x00, %bh                 # Halaman 0
    int $BIOS_VIDEO
    ret

# =============================================================================
# PENANGAN ERROR
# =============================================================================

_gagal_disk:
    movw $._pesan_gagal_disk, %si
    call _tampilkan_string
    
    # Tampilkan kode error
    movb %ah, %al                   # Kode error di AH
    call _tampilkan_hex
    
    jmp _gagal

_gagal_kosong:
    movw $._pesan_gagal_kosong, %si
    call _tampilkan_string
    jmp _gagal

_gagal:
    movw $._pesan_gagal, %si
    call _tampilkan_string
    
    # Tunggu tombol ditekan
    xorw %ax, %ax
    int $0x16                       # BIOS keyboard input
    
    # Restart sistem
    movw $0x1234, 0x472             # Warm boot flag
    ljmp $0xFFFF, $0x0000           # Reset vector

# =============================================================================
# DATA
# =============================================================================

.section .data

._pesan_judul:
    .asciz "\r\n\033[36mPIGURA OS v1.0 - Boot Sector\033[0m\r\n"

._pesan_muat:
    .asciz "\r\nMemuat stage 2 bootloader...\r\n"

._pesan_sukses:
    .asciz "Stage 2 berhasil dimuat.\r\n"

._pesan_gagal_disk:
    .asciz "\r\n\033[31mERROR: Gagal membaca disk. Kode: "

._pesan_gagal_kosong:
    .asciz "\r\n\033[31mERROR: Stage 2 kosong.\033[0m\r\n"

._pesan_gagal:
    .asciz "\033[0m\r\nTekan tombol untuk restart...\r\n"

.section .bss
_drive_boot:
    .skip 1                         # Tempat menyimpan drive boot

# =============================================================================
# BOOT SIGNATURE
# =============================================================================
# Boot sector harus diakhiri dengan signature 0xAA55
# Total ukuran harus tepat 512 byte
# =============================================================================

.section .text
._pad:
    # Padding hingga 510 byte
    . = _start + 510
    .word 0xAA55                    # Boot signature
