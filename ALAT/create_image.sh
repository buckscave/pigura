#!/bin/bash
#
# PIGURA OS - SKRIP CREATE IMAGE
# ------------------------------
# Skrip ini membuat disk image bootable untuk Pigura OS.
#
# Penggunaan: ./create_image.sh [arsitektur] [tipe] [opsi]
#   arsitektur: x86, x86_64, arm, armv7, arm64
#   tipe: raw, iso, vdi, qcow2
#
# Versi: 1.0
# Tanggal: 2025

# ========================================
# KONFIGURASI
# ========================================

DIREKTORI_AKAR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIREKTORI_OUTPUT="${DIREKTORI_AKAR}/OUTPUT"
DIREKTORI_IMAGE="${DIREKTORI_AKAR}/IMAGE"
DIREKTORI_ISO="${DIREKTORI_AKAR}/ISO"

# Ukuran image default
UKURAN_IMAGE_DEFAULT="64M"

# Warna untuk output
WARNA_RESET="\033[0m"
WARNA_MERAH="\033[31m"
WARNA_HIJAU="\033[32m"
WARNA_KUNING="\033[33m"
WARNA_BIRU="\033[34m"

# ========================================
# FUNGSI UTILITAS
# ========================================

cetak_info() {
    echo -e "${WARNA_BIRU}[INFO]${WARNA_RESET} $1"
}

cetak_sukses() {
    echo -e "${WARNA_HIJAU}[SUKSES]${WARNA_RESET} $1"
}

cetak_peringatan() {
    echo -e "${WARNA_KUNING}[PERINGATAN]${WARNA_RESET} $1"
}

cetak_error() {
    echo -e "${WARNA_MERAH}[ERROR]${WARNA_RESET} $1"
}

perintah_tersedia() {
    command -v "$1" >/dev/null 2>&1
}

# ========================================
# FUNGSI IMAGE CREATION
# ========================================

# Buat direktori output
buat_direktori() {
    mkdir -p "$DIREKTORI_IMAGE"
    mkdir -p "$DIREKTORI_ISO"
}

# Buat raw disk image
buat_raw_image() {
    local arsitektur=$1
    local ukuran=$2
    local output="${DIREKTORI_IMAGE}/pigura_${arsitektur}.img"

    cetak_info "Membuat raw disk image..."
    cetak_info "Ukuran: $ukuran"
    cetak_info "Output: $output"

    # Buat file kosong
    dd if=/dev/zero of="$output" bs=1M count=$(echo $ukuran | sed 's/M//') 2>/dev/null

    if [ $? -ne 0 ]; then
        cetak_error "Gagal membuat image"
        return 1
    fi

    cetak_sukses "Raw image dibuat: $output"
    echo "$output"
    return 0
}

# Buat disk image dengan partisi
buat_image_partisi() {
    local arsitektur=$1
    local ukuran=$2
    local output="${DIREKTORI_IMAGE}/pigura_${arsitektur}_part.img"

    cetak_info "Membuat disk image dengan partisi..."

    # Buat image
    dd if=/dev/zero of="$output" bs=1M count=$(echo $ukuran | sed 's/M//') 2>/dev/null

    # Buat partition table (MBR)
    if perintah_tersedia "parted"; then
        parted -s "$output" mklabel msdos
        parted -s "$output" mkpart primary fat32 1MiB 100%
        parted -s "$output" set 1 boot on
        cetak_sukses "Partisi dibuat"
    else
        cetak_peringatan "parted tidak tersedia, skip pembuatan partisi"
    fi

    cetak_sukses "Image dengan partisi: $output"
    echo "$output"
}

# Buat ISO image
buat_iso_image() {
    local arsitektur=$1
    local kernel="${DIREKTORI_OUTPUT}/pigura_${arsitektur}.elf"
    local output="${DIREKTORI_ISO}/pigura_${arsitektur}.iso"
    local tmp_dir=$(mktemp -d)

    cetak_info "Membuat ISO image..."

    if [ ! -f "$kernel" ]; then
        cetak_error "Kernel tidak ditemukan: $kernel"
        rm -rf "$tmp_dir"
        return 1
    fi

    # Struktur direktori ISO
    mkdir -p "${tmp_dir}/boot/grub"

    # Copy kernel
    cp "$kernel" "${tmp_dir}/boot/kernel.elf"

    # Buat GRUB config
    cat > "${tmp_dir}/boot/grub/grub.cfg" << 'EOF'
set timeout=5
set default=0

menuentry "Pigura OS" {
    multiboot /boot/kernel.elf
    boot
}

menuentry "Pigura OS (Debug Mode)" {
    multiboot /boot/kernel.elf debug=1
    boot
}

menuentry "Pigura OS (Safe Mode)" {
    multiboot /boot/kernel.elf safe=1
    boot
}
EOF

    # Buat ISO dengan grub-mkrescue
    if perintah_tersedia "grub-mkrescue"; then
        grub-mkrescue -o "$output" "$tmp_dir" 2>/dev/null
        cetak_sukses "ISO image: $output"
    elif perintah_tersedia "xorriso"; then
        xorriso -as mkisofs -o "$output" -R -J "$tmp_dir" 2>/dev/null
        cetak_sukses "ISO image (basic): $output"
    else
        cetak_error "grub-mkrescue/xorriso tidak tersedia"
        cetak_info "Install dengan: sudo apt install grub-pc-bin xorriso"
        rm -rf "$tmp_dir"
        return 1
    fi

    rm -rf "$tmp_dir"
    echo "$output"
}

# Buat VDI image (VirtualBox)
buat_vdi_image() {
    local arsitektur=$1
    local ukuran=$2
    local raw_image="${DIREKTORI_IMAGE}/pigura_${arsitektur}.img"
    local output="${DIREKTORI_IMAGE}/pigura_${arsitektur}.vdi"

    cetak_info "Membuat VDI image..."

    if ! perintah_tersedia "VBoxManage"; then
        cetak_error "VirtualBox tidak terinstall"
        cetak_info "Install VirtualBox untuk membuat VDI"
        return 1
    fi

    # Buat raw image dulu jika belum ada
    if [ ! -f "$raw_image" ]; then
        raw_image=$(buat_raw_image "$arsitektur" "$ukuran")
    fi

    # Convert ke VDI
    VBoxManage convertfromraw "$raw_image" "$output" 2>/dev/null

    if [ $? -eq 0 ]; then
        cetak_sukses "VDI image: $output"
        echo "$output"
    else
        cetak_error "Gagal membuat VDI"
        return 1
    fi
}

# Buat QCOW2 image (QEMU)
buat_qcow2_image() {
    local arsitektur=$1
    local ukuran=$2
    local output="${DIREKTORI_IMAGE}/pigura_${arsitektur}.qcow2"

    cetak_info "Membuat QCOW2 image..."

    if ! perintah_tersedia "qemu-img"; then
        cetak_error "qemu-img tidak tersedia"
        cetak_info "Install dengan: sudo apt install qemu-utils"
        return 1
    fi

    qemu-img create -f qcow2 "$output" "$ukuran" 2>/dev/null

    if [ $? -eq 0 ]; then
        cetak_sukses "QCOW2 image: $output"
        echo "$output"
    else
        cetak_error "Gagal membuat QCOW2"
        return 1
    fi
}

# Install bootloader ke image
install_bootloader() {
    local image=$1
    local arsitektur=$2
    local kernel="${DIREKTORI_OUTPUT}/pigura_${arsitektur}.elf"

    cetak_info "Menginstall bootloader..."

    if [ "$arsitektur" = "x86" ] || [ "$arsitektur" = "x86_64" ]; then
        # Install GRUB atau syslinux
        if perintah_tersedia "syslinux"; then
            install_syslinux "$image" "$kernel"
        else
            cetak_peringatan "syslinux tidak tersedia"
            cetak_info "Install dengan: sudo apt install syslinux"
        fi
    else
        cetak_peringatan "Bootloader otomatis belum didukung untuk $arsitektur"
        cetak_info "Gunakan -kernel option di QEMU"
    fi
}

# Install syslinux
install_syslinux() {
    local image=$1
    local kernel=$2

    cetak_info "Installing syslinux..."

    # Setup loop device
    local loop=$(losetup -f --show "$image" 2>/dev/null)

    if [ -z "$loop" ]; then
        cetak_error "Gagal setup loop device (coba dengan sudo)"
        return 1
    fi

    # Install MBR
    local mbr="/usr/lib/syslinux/mbr/mbr.bin"
    if [ -f "$mbr" ]; then
        dd if="$mbr" of="$image" bs=440 count=1 conv=notrunc 2>/dev/null
    fi

    # Cleanup
    losetup -d "$loop" 2>/dev/null

    cetak_sukses "Syslinux installed"
}

# Buat initramfs
buat_initramfs() {
    local arsitektur=$1
    local output="${DIREKTORI_OUTPUT}/initramfs_${arsitektur}.cpio.gz"
    local tmp_dir=$(mktemp -d)

    cetak_info "Membuat initramfs..."

    # Struktur direktori
    mkdir -p "${tmp_dir}/bin"
    mkdir -p "${tmp_dir}/sbin"
    mkdir -p "${tmp_dir}/dev"
    mkdir -p "${tmp_dir}/proc"
    mkdir -p "${tmp_dir}/sys"
    mkdir -p "${tmp_dir}/etc"
    mkdir -p "${tmp_dir}/lib"
    mkdir -p "${tmp_dir}/tmp"
    mkdir -p "${tmp_dir}/var"
    mkdir -p "${tmp_dir}/root"

    # Buat init script
    cat > "${tmp_dir}/init" << 'EOF'
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
exec /bin/sh
EOF
    chmod +x "${tmp_dir}/init"

    # Buat device nodes
    mknod "${tmp_dir}/dev/console" c 5 1 2>/dev/null
    mknod "${tmp_dir}/dev/null" c 1 3 2>/dev/null
    mknod "${tmp_dir}/dev/tty" c 5 0 2>/dev/null

    # Create cpio archive
    (cd "$tmp_dir" && find . | cpio -H newc -o 2>/dev/null) | gzip > "$output"

    rm -rf "$tmp_dir"

    cetak_sukses "Initramfs: $output"
    echo "$output"
}

# ========================================
# BANTUAN
# ========================================

tampilkan_bantuan() {
    echo ""
    echo "PIGURA OS - Skrip Create Image"
    echo "==============================="
    echo ""
    echo "Penggunaan: $0 [arsitektur] [tipe] [opsi]"
    echo ""
    echo "Arsitektur:"
    echo "  x86      - Intel/AMD 32-bit"
    echo "  x86_64   - Intel/AMD 64-bit"
    echo "  arm      - ARM 32-bit"
    echo "  armv7    - ARMv7 32-bit"
    echo "  arm64    - ARM64 64-bit"
    echo ""
    echo "Tipe Image:"
    echo "  raw      - Raw disk image (default)"
    echo "  iso      - ISO 9660 (bootable CD)"
    echo "  vdi      - VirtualBox disk image"
    echo "  qcow2    - QEMU disk image"
    echo "  part     - Image dengan partisi"
    echo "  initrd   - Initramfs/Initrd"
    echo "  all      - Semua tipe"
    echo ""
    echo "Opsi:"
    echo "  -s SIZE  - Ukuran image (default: 64M)"
    echo "  -o FILE  - File output"
    echo "  -b       - Install bootloader"
    echo "  -h       - Tampilkan bantuan"
    echo ""
    echo "Contoh:"
    echo "  $0 x86 raw            # Raw image 64MB"
    echo "  $0 x86_64 iso         # ISO image"
    echo "  $0 arm64 qcow2 -s 128M  # QCOW2 128MB"
    echo "  $0 x86_64 all         # Semua tipe"
    echo ""
}

# ========================================
# MAIN
# ========================================

main() {
    local arsitektur=""
    local tipe="raw"
    local ukuran="$UKURAN_IMAGE_DEFAULT"
    local output=""
    local install_bl=0

    # Parse argumen
    while [ $# -gt 0 ]; do
        case "$1" in
            -s)
                ukuran=$2
                shift 2
                ;;
            -o)
                output=$2
                shift 2
                ;;
            -b)
                install_bl=1
                shift
                ;;
            -h|--help|help)
                tampilkan_bantuan
                exit 0
                ;;
            raw|iso|vdi|qcow2|part|initrd|all)
                tipe=$1
                shift
                ;;
            x86|x86_64|arm|armv7|arm64)
                arsitektur=$1
                shift
                ;;
            *)
                cetak_error "Argumen tidak dikenali: $1"
                tampilkan_bantuan
                exit 1
                ;;
        esac
    done

    # Validasi
    if [ -z "$arsitektur" ]; then
        cetak_peringatan "Arsitektur tidak ditentukan, menggunakan x86"
        arsitektur="x86"
    fi

    # Buat direktori
    buat_direktori

    # Cek kernel
    local kernel="${DIREKTORI_OUTPUT}/pigura_${arsitektur}.elf"
    if [ ! -f "$kernel" ] && [ "$tipe" != "initrd" ]; then
        cetak_error "Kernel tidak ditemukan: $kernel"
        cetak_info "Jalankan build.sh terlebih dahulu"
        exit 1
    fi

    cetak_info "=========================================="
    cetak_info "  PIGURA OS - Create Image"
    cetak_info "=========================================="
    cetak_info "Arsitektur: $arsitektur"
    cetak_info "Tipe:       $tipe"
    cetak_info "Ukuran:     $ukuran"
    cetak_info "=========================================="

    # Execute
    case "$tipe" in
        raw)
            local img=$(buat_raw_image "$arsitektur" "$ukuran")
            if [ $install_bl -eq 1 ] && [ -n "$img" ]; then
                install_bootloader "$img" "$arsitektur"
            fi
            ;;
        part)
            buat_image_partisi "$arsitektur" "$ukuran"
            ;;
        iso)
            buat_iso_image "$arsitektur"
            ;;
        vdi)
            buat_vdi_image "$arsitektur" "$ukuran"
            ;;
        qcow2)
            buat_qcow2_image "$arsitektur" "$ukuran"
            ;;
        initrd)
            buat_initramfs "$arsitektur"
            ;;
        all)
            buat_raw_image "$arsitektur" "$ukuran"
            buat_iso_image "$arsitektur"
            buat_qcow2_image "$arsitektur" "$ukuran"
            buat_initramfs "$arsitektur"
            ;;
        *)
            cetak_error "Tipe tidak dikenali: $tipe"
            exit 1
            ;;
    esac

    cetak_sukses "Image creation selesai"
}

# Run
main "$@"
