#!/bin/bash
#
# PIGURA OS - SKRIP QEMU RUN
# --------------------------
# Skrip ini menjalankan kernel Pigura OS di emulator QEMU.
#
# Penggunaan: ./qemu_run.sh [arsitektur] [opsi]
#   arsitektur: x86, x86_64, arm, armv7, arm64
#
# Versi: 1.0
# Tanggal: 2025

# ========================================
# KONFIGURASI
# ========================================

DIREKTORI_AKAR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIREKTORI_OUTPUT="${DIREKTORI_AKAR}/OUTPUT"
DIREKTORI_IMAGE="${DIREKTORI_AKAR}/IMAGE"

# Default settings
MEMORI_DEFAULT="512M"
DISPLAY_DEFAULT="none"
SERIAL_DEFAULT="stdio"

# Warna untuk output
WARNA_RESET="\033[0m"
WARNA_MERAH="\033[31m"
WARNA_HIJAU="\033[32m"
WARNA_KUNING="\033[33m"
WARNA_BIRU="\033[34m"
WARNA_CYAN="\033[36m"

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
# KONFIGURASI PER ARSITEKTUR
# ========================================

dapatkan_qemu() {
    local arsitektur=$1

    case "$arsitektur" in
        x86)
            echo "qemu-system-i386"
            ;;
        x86_64)
            echo "qemu-system-x86_64"
            ;;
        arm)
            echo "qemu-system-arm"
            ;;
        armv7)
            echo "qemu-system-arm"
            ;;
        arm64)
            echo "qemu-system-aarch64"
            ;;
        *)
            echo ""
            ;;
    esac
}

dapatkan_machine() {
    local arsitektur=$1

    case "$arsitektur" in
        x86)
            echo "q35"  # Modern machine type
            ;;
        x86_64)
            echo "q35"
            ;;
        arm)
            echo "versatilepb"  # Legacy ARM
            ;;
        armv7)
            echo "virt"  # Virtual ARM platform
            ;;
        arm64)
            echo "virt"
            ;;
        *)
            echo ""
            ;;
    esac
}

dapatkan_cpu() {
    local arsitektur=$1

    case "$arsitektur" in
        x86)
            echo "qemu32,+pae"
            ;;
        x86_64)
            echo "qemu64"
            ;;
        arm)
            echo "arm926"
            ;;
        armv7)
            echo "cortex-a15"
            ;;
        arm64)
            echo "cortex-a57"
            ;;
        *)
            echo ""
            ;;
    esac
}

dapatkan_kernel() {
    local arsitektur=$1

    local kernel="${DIREKTORI_OUTPUT}/pigura_${arsitektur}.elf"

    if [ ! -f "$kernel" ]; then
        # Cek format lain
        kernel="${DIREKTORI_OUTPUT}/pigura_${arsitektur}.bin"
    fi

    if [ ! -f "$kernel" ]; then
        # Cek disk image
        kernel="${DIREKTORI_IMAGE}/pigura_${arsitektur}.img"
    fi

    echo "$kernel"
}

# ========================================
# FUNGSI QEMU
# ========================================

# Jalankan QEMU mode teks
jalankan_teks() {
    local arsitektur=$1
    local kernel=$2
    local mem=$3

    local qemu=$(dapatkan_qemu "$arsitektur")
    local machine=$(dapatkan_machine "$arsitektur")
    local cpu=$(dapatkan_cpu "$arsitektur")

    if ! perintah_tersedia "$qemu"; then
        cetak_error "QEMU tidak ditemukan: $qemu"
        cetak_info "Install dengan: sudo apt install qemu-system-${arsitektur/x86_64/x86}"
        return 1
    fi

    cetak_info "=========================================="
    cetak_info "  PIGURA OS - QEMU Emulator"
    cetak_info "=========================================="
    cetak_info "Arsitektur: $arsitektur"
    cetak_info "Machine:    $machine"
    cetak_info "CPU:        $cpu"
    cetak_info "Memory:     $mem"
    cetak_info "Kernel:     $kernel"
    cetak_info "=========================================="
    cetak_info ""
    cetak_info "Tekan Ctrl+A lalu X untuk keluar"
    cetak_info ""

    # Argumen QEMU
    local args="-machine $machine"
    args="$args -cpu $cpu"
    args="$args -m $mem"
    args="$args -kernel $kernel"

    # Serial console
    args="$args -nographic"
    args="$args -serial mon:stdio"

    # No reboot/shutdown
    args="$args -no-reboot"
    args="$args -no-shutdown"

    # x86 specific
    if [ "$arsitektur" = "x86" ] || [ "$arsitektur" = "x86_64" ]; then
        args="$args -vga none"
        args="$args -display none"
    fi

    # ARM specific
    if [ "$arsitektur" = "arm" ]; then
        # VersatilePB specific
        args="$args"
    elif [ "$arsitektur" = "armv7" ] || [ "$arsitektur" = "arm64" ]; then
        # Virt machine
        args="$args -semihosting"
    fi

    exec $qemu $args
}

# Jalankan QEMU mode grafis
jalankan_grafis() {
    local arsitektur=$1
    local kernel=$2
    local mem=$3
    local resolution=$4

    local qemu=$(dapatkan_qemu "$arsitektur")
    local machine=$(dapatkan_machine "$arsitektur")
    local cpu=$(dapatkan_cpu "$arsitektur")

    if ! perintah_tersedia "$qemu"; then
        cetak_error "QEMU tidak ditemukan: $qemu"
        return 1
    fi

    cetak_info "=========================================="
    cetak_info "  PIGURA OS - QEMU (Grafis)"
    cetak_info "=========================================="
    cetak_info "Arsitektur: $arsitektur"
    cetak_info "Resolution: $resolution"
    cetak_info "Memory:     $mem"
    cetak_info "=========================================="

    # Argumen QEMU
    local args="-machine $machine"
    args="$args -cpu $cpu"
    args="$args -m $mem"
    args="$args -kernel $kernel"

    # Display
    args="$args -vga std"

    # Resolution
    if [ -n "$resolution" ]; then
        args="$args -gdk-scale 1"
    fi

    # Serial ke stdio untuk log
    args="$args -serial stdio"

    # Input devices
    args="$args -usb"
    args="$args -device usb-mouse"
    args="$args -device usb-kbd"

    # Network (optional)
    args="$args -net nic"
    args="$args -net user"

    exec $qemu $args
}

# Jalankan dengan disk image
jalankan_image() {
    local arsitektur=$1
    local image=$2
    local mem=$3

    local qemu=$(dapatkan_qemu "$arsitektur")
    local machine=$(dapatkan_machine "$arsitektur")

    if [ ! -f "$image" ]; then
        cetak_error "Image tidak ditemukan: $image"
        return 1
    fi

    cetak_info "=========================================="
    cetak_info "  PIGURA OS - Boot dari Image"
    cetak_info "=========================================="
    cetak_info "Image: $image"
    cetak_info "=========================================="

    local args="-machine $machine"
    args="$args -m $mem"
    args="$args -drive file=$image,format=raw,if=ide"
    args="$args -nographic"
    args="$args -serial mon:stdio"

    exec $qemu $args
}

# Jalankan dengan UEFI
jalankan_uefi() {
    local arsitektur=$1
    local kernel=$2
    local mem=$3

    if [ "$arsitektur" != "x86_64" ] && [ "$arsitektur" != "arm64" ]; then
        cetak_error "UEFI hanya didukung untuk x86_64 dan arm64"
        return 1
    fi

    local qemu=$(dapatkan_qemu "$arsitektur")

    # Cek OVMF untuk x86_64
    local ovmf=""
    if [ "$arsitektur" = "x86_64" ]; then
        if [ -f "/usr/share/OVMF/OVMF_CODE.fd" ]; then
            ovmf="/usr/share/OVMF/OVMF_CODE.fd"
        elif [ -f "/usr/share/ovmf/OVMF.fd" ]; then
            ovmf="/usr/share/ovmf/OVMF.fd"
        else
            cetak_error "OVMF tidak ditemukan"
            cetak_info "Install dengan: sudo apt install ovmf"
            return 1
        fi
    fi

    cetak_info "=========================================="
    cetak_info "  PIGURA OS - UEFI Boot"
    cetak_info "=========================================="

    local args="-machine q35,accel=tcg"
    args="$args -cpu qemu64"
    args="$args -m $mem"
    args="$args -kernel $kernel"

    if [ -n "$ovmf" ]; then
        args="$args -drive if=pflash,format=raw,readonly=on,file=$ovmf"
    fi

    args="$args -nographic"
    args="$args -serial mon:stdio"

    exec $qemu $args
}

# ========================================
# BANTUAN
# ========================================

tampilkan_bantuan() {
    echo ""
    echo "PIGURA OS - Skrip QEMU Run"
    echo "==========================="
    echo ""
    echo "Penggunaan: $0 [arsitektur] [mode] [opsi]"
    echo ""
    echo "Arsitektur:"
    echo "  x86      - Intel/AMD 32-bit"
    echo "  x86_64   - Intel/AMD 64-bit"
    echo "  arm      - ARM 32-bit (legacy)"
    echo "  armv7    - ARMv7 32-bit"
    echo "  arm64    - ARM64 64-bit"
    echo ""
    echo "Mode:"
    echo "  text     - Mode teks (console serial)"
    echo "  graphic  - Mode grafis"
    echo "  image    - Boot dari disk image"
    echo "  uefi     - Boot via UEFI (x86_64/arm64)"
    echo ""
    echo "Opsi:"
    echo "  -m SIZE  - Ukuran memori (default: 512M)"
    echo "  -i FILE  - File disk image"
    echo "  -r WxH   - Resolusi layar (grafis)"
    echo "  -h       - Tampilkan bantuan"
    echo ""
    echo "Contoh:"
    echo "  $0 x86                    # Mode teks x86"
    echo "  $0 x86_64 graphic         # Mode grafis"
    echo "  $0 arm64 -m 1G            # 1GB memori"
    echo "  $0 x86_64 image -i disk.img"
    echo ""
    echo "Keyboard shortcuts di QEMU:"
    echo "  Ctrl+A X  - Keluar"
    echo "  Ctrl+A C  - Toggle console"
    echo ""
}

# ========================================
# MAIN
# ========================================

main() {
    local arsitektur=""
    local mode="text"
    local memori="$MEMORI_DEFAULT"
    local image=""
    local resolution=""

    # Parse argumen
    while [ $# -gt 0 ]; do
        case "$1" in
            -m)
                memori=$2
                shift 2
                ;;
            -i)
                image=$2
                shift 2
                ;;
            -r)
                resolution=$2
                shift 2
                ;;
            -h|--help|help)
                tampilkan_bantuan
                exit 0
                ;;
            text|graphic|image|uefi)
                mode=$1
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

    # Validasi arsitektur
    if [ -z "$arsitektur" ]; then
        cetak_peringatan "Arsitektur tidak ditentukan, menggunakan x86"
        arsitektur="x86"
    fi

    # Dapatkan kernel
    local kernel=$(dapatkan_kernel "$arsitektur")

    if [ ! -f "$kernel" ]; then
        cetak_error "Kernel tidak ditemukan untuk $arsitektur"
        cetak_info "Jalankan build.sh terlebih dahulu"
        exit 1
    fi

    # Execute mode
    case "$mode" in
        text)
            jalankan_teks "$arsitektur" "$kernel" "$memori"
            ;;
        graphic)
            jalankan_grafis "$arsitektur" "$kernel" "$memori" "$resolution"
            ;;
        image)
            if [ -z "$image" ]; then
                image="${DIREKTORI_IMAGE}/pigura_${arsitektur}.img"
            fi
            jalankan_image "$arsitektur" "$image" "$memori"
            ;;
        uefi)
            jalankan_uefi "$arsitektur" "$kernel" "$memori"
            ;;
        *)
            cetak_error "Mode tidak dikenali: $mode"
            exit 1
            ;;
    esac
}

# Run
main "$@"
