#!/bin/bash
#
# PIGURA OS - SKRIP DEBUGGING
# ---------------------------
# Skrip ini menjalankan kernel di QEMU dengan GDB untuk
# keperluan debugging.
#
# Penggunaan: ./debug.sh [arsitektur] [opsi]
#   arsitektur: x86, x86_64, arm, armv7, arm64
#
# Versi: 1.0
# Tanggal: 2025

# ========================================
# KONFIGURASI
# ========================================

DIREKTORI_AKAR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIREKTORI_OUTPUT="${DIREKTORI_AKAR}/OUTPUT"

# Port GDB
GDB_PORT=1234

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
# KONFIGURASI QEMU
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
        x86|x86_64)
            echo "q35"
            ;;
        arm)
            echo "versatilepb"
            ;;
        armv7)
            echo "virt"
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
            echo "qemu32"
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

# ========================================
# FUNGSI DEBUG
# ========================================

# Cek kernel file
cek_kernel() {
    local arsitektur=$1
    local kernel="${DIREKTORI_OUTPUT}/pigura_${arsitektur}.elf"

    if [ ! -f "$kernel" ]; then
        cetak_error "Kernel tidak ditemukan: $kernel"
        cetak_info "Jalankan build.sh terlebih dahulu"
        return 1
    fi

    echo "$kernel"
    return 0
}

# Jalankan dengan QEMU + GDB server
jalankan_debug_server() {
    local arsitektur=$1
    local kernel=$2

    local qemu=$(dapatkan_qemu "$arsitektur")
    local machine=$(dapatkan_machine "$arsitektur")
    local cpu=$(dapatkan_cpu "$arsitektur")

    if ! perintah_tersedia "$qemu"; then
        cetak_error "QEMU tidak ditemukan: $qemu"
        return 1
    fi

    cetak_info "Memulai QEMU dengan GDB server di port $GDB_PORT"
    cetak_info "Kernel: $kernel"
    cetak_info "Machine: $machine"
    cetak_info "CPU: $cpu"

    # Argumen QEMU dasar
    local args="-machine $machine"
    args="$args -cpu $cpu"
    args="$args -m 256M"
    args="$args -kernel $kernel"
    args="$args -s -S"  # -s = gdb server, -S = pause di awal
    args="$args -nographic"
    args="$args -serial mon:stdio"

    # Tambahan untuk x86/x86_64
    if [ "$arsitektur" = "x86" ] || [ "$arsitektur" = "x86_64" ]; then
        args="$args -no-reboot"
        args="$args -no-shutdown"
    fi

    # Tambahan untuk ARM
    if [ "$arsitektur" = "arm" ] || [ "$arsitektur" = "armv7" ]; then
        args="$args -semihosting"
    fi

    cetak_info "Menjalankan: $qemu $args"
    echo ""
    cetak_info "Tekan Ctrl+A lalu X untuk keluar dari QEMU"
    echo ""

    exec $qemu $args
}

# Jalankan GDB client
jalankan_gdb() {
    local arsitektur=$1
    local kernel=$2

    local gdb="gdb"

    # Gunakan gdb multiarch jika tersedia
    if perintah_tersedia "gdb-multiarch"; then
        gdb="gdb-multiarch"
    fi

    if ! perintah_tersedia "$gdb"; then
        cetak_error "GDB tidak ditemukan"
        cetak_info "Install gdb atau gdb-multiarch"
        return 1
    fi

    cetak_info "Menjalankan GDB..."
    cetak_info "Target: localhost:$GDB_PORT"
    cetak_info "Symbol: $kernel"

    # Buat file GDB init sementara
    local gdbinit=$(mktemp)
    cat > "$gdbinit" << EOF
set architecture auto
target remote localhost:$GDB_PORT
symbol-file $kernel

# Breakpoints dasar
break _start
break kernel_main
break panic

# Commands
define list_breakpoints
    info breakpoints
end

define step_instruction
    stepi
end

define continue_execution
    continue
end

echo \\n
echo ========================================\\n
echo PIGURA OS DEBUG SESSION\\n
echo ========================================\\n
echo \\n
echo Perintah berguna:\\n
echo   c         - continue execution\\n
echo   si        - step instruction\\n
echo   n         - next line\\n
echo   s         - step into\\n
echo   bt        - backtrace\\n
echo   info reg  - register info\\n
echo   x/10i \$pc - disassemble\\n
echo ========================================\\n
echo \\n
EOF

    $gdb -x "$gdbinit"

    rm -f "$gdbinit"
}

# ========================================
# FUNGSI SYMBOL DUMP
# ========================================

dump_symbols() {
    local arsitektur=$1
    local kernel="${DIREKTORI_OUTPUT}/pigura_${arsitektur}.elf"

    if [ ! -f "$kernel" ]; then
        cetak_error "Kernel tidak ditemukan"
        return 1
    fi

    cetak_info "Symbol table untuk $arsitektur"
    echo ""

    # Gunakan nm atau objdump
    if perintah_tersedia "nm"; then
        nm "$kernel" | sort | head -50
    elif perintah_tersedia "objdump"; then
        objdump -t "$kernel" | head -50
    else
        cetak_error "nm/objdump tidak tersedia"
    fi
}

# Dump section headers
dump_sections() {
    local arsitektur=$1
    local kernel="${DIREKTORI_OUTPUT}/pigura_${arsitektur}.elf"

    if [ ! -f "$kernel" ]; then
        cetak_error "Kernel tidak ditemukan"
        return 1
    fi

    cetak_info "Section headers untuk $arsitektur"
    echo ""

    if perintah_tersedia "objdump"; then
        objdump -h "$kernel"
    elif perintah_tersedia "readelf"; then
        readelf -S "$kernel"
    else
        cetak_error "objdump/readelf tidak tersedia"
    fi
}

# ========================================
# BANTUAN
# ========================================

tampilkan_bantuan() {
    echo ""
    echo "PIGURA OS - Skrip Debugging"
    echo "============================"
    echo ""
    echo "Penggunaan: $0 [arsitektur] [mode] [opsi]"
    echo ""
    echo "Arsitektur:"
    echo "  x86      - Intel/AMD 32-bit"
    echo "  x86_64   - Intel/AMD 64-bit"
    echo "  arm      - ARM 32-bit"
    echo "  armv7    - ARMv7 32-bit"
    echo "  arm64    - ARM64 64-bit"
    echo ""
    echo "Mode:"
    echo "  server   - Jalankan QEMU dengan GDB server"
    echo "  gdb      - Jalankan GDB client"
    echo "  symbols  - Dump symbol table"
    echo "  sections - Dump section headers"
    echo "  run      - Jalankan tanpa debug"
    echo ""
    echo "Opsi:"
    echo "  -p PORT  - Port GDB (default: 1234)"
    echo "  -h       - Tampilkan bantuan"
    echo ""
    echo "Contoh:"
    echo "  $0 x86 server           # QEMU debug server x86"
    echo "  $0 x86 gdb              # GDB client x86"
    echo "  $0 x86_64 symbols       # Lihat symbols x86_64"
    echo ""
    echo "Workflow debugging:"
    echo "  1. Terminal 1: $0 x86 server"
    echo "  2. Terminal 2: $0 x86 gdb"
    echo ""
}

# ========================================
# MAIN
# ========================================

main() {
    local arsitektur=""
    local mode="server"

    # Parse argumen
    while [ $# -gt 0 ]; do
        case "$1" in
            -p)
                GDB_PORT=$2
                shift 2
                ;;
            -h|--help|help)
                tampilkan_bantuan
                exit 0
                ;;
            server|gdb|symbols|sections|run)
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

    # Cek kernel
    local kernel=$(cek_kernel "$arsitektur")
    if [ $? -ne 0 ]; then
        exit 1
    fi

    # Execute mode
    case "$mode" in
        server)
            jalankan_debug_server "$arsitektur" "$kernel"
            ;;
        gdb)
            jalankan_gdb "$arsitektur" "$kernel"
            ;;
        symbols)
            dump_symbols "$arsitektur"
            ;;
        sections)
            dump_sections "$arsitektur"
            ;;
        run)
            # Run without debug
            QEMU_FLAGS="" "${DIREKTORI_AKAR}/ALAT/qemu_run.sh" "$arsitektur"
            ;;
        *)
            cetak_error "Mode tidak dikenali: $mode"
            exit 1
            ;;
    esac
}

# Run
main "$@"
