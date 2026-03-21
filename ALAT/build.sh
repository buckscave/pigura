#!/bin/bash
#
# PIGURA OS - SKRIP BUILD UTAMA
# -----------------------------
# Skrip ini mengkompilasi kernel Pigura OS untuk berbagai
# arsitektur yang didukung.
#
# Penggunaan: ./build.sh [arsitektur] [target]
#   arsitektur: x86, x86_64, arm, armv7, arm64
#   target: all, kernel, bootloader, clean
#
# Versi: 1.0
# Tanggal: 2025

# ========================================
# KONFIGURASI
# ========================================

# Direktori
DIREKTORI_AKAR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIREKTORI_SUMBER="${DIREKTORI_AKAR}/SUMBER"
DIREKTORI_ALAT="${DIREKTORI_AKAR}/ALAT"
DIREKTORI_BUILD="${DIREKTORI_AKAR}/BUILD"
DIREKTORI_OUTPUT="${DIREKTORI_AKAR}/OUTPUT"

# Versi compiler minimum
VERSIGCC_MINIMUM=7
VERSICLANG_MINIMUM=9

# Warna untuk output
WARNA_RESET="\033[0m"
WARNA_MERAH="\033[31m"
WARNA_HIJAU="\033[32m"
WARNA_KUNING="\033[33m"
WARNA_BIRU="\033[34m"
WARNA_UNGU="\033[35m"
WARNA_CYAN="\033[36m"

# ========================================
# FUNGSI UTILITAS
# ========================================

# Cetak pesan dengan warna
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

cetak_header() {
    echo ""
    echo -e "${WARNA_UNGU}========================================${WARNA_RESET}"
    echo -e "${WARNA_UNGU}  $1${WARNA_RESET}"
    echo -e "${WARNA_UNGU}========================================${WARNA_RESET}"
    echo ""
}

# Cek apakah perintah tersedia
perintah_tersedia() {
    command -v "$1" >/dev/null 2>&1
}

# Dapatkan versi GCC
dapatkan_versi_gcc() {
    if perintah_tersedia gcc; then
        gcc --version | head -n1 | grep -oP '\d+\.\d+' | head -1 | cut -d. -f1
    else
        echo "0"
    fi
}

# Dapatkan versi Clang
dapatkan_versi_clang() {
    if perintah_tersedia clang; then
        clang --version | head -n1 | grep -oP '\d+' | head -1
    else
        echo "0"
    fi
}

# ========================================
# FUNGSI KOMPILATOR
# ========================================

# Dapatkan compiler berdasarkan arsitektur
dapatkan_compiler() {
    local arsitektur=$1
    local cc=""
    local as=""
    local ld=""

    case "$arsitektur" in
        x86)
            cc="gcc"
            as="as"
            ld="ld"
            cflags="-m32 -march=i686"
            ldflags="-m elf_i386"
            ;;
        x86_64)
            cc="gcc"
            as="as"
            ld="ld"
            cflags="-m64 -march=x86-64"
            ldflags="-m elf_x86_64"
            ;;
        arm)
            if perintah_tersedia arm-linux-gnueabi-gcc; then
                cc="arm-linux-gnueabi-gcc"
                as="arm-linux-gnueabi-as"
                ld="arm-linux-gnueabi-ld"
            elif perintah_tersedia arm-none-eabi-gcc; then
                cc="arm-none-eabi-gcc"
                as="arm-none-eabi-as"
                ld="arm-none-eabi-ld"
            else
                cc="arm-gcc"
                as="arm-as"
                ld="arm-ld"
            fi
            cflags="-marm -march=armv6 -mfloat-abi=soft"
            ldflags=""
            ;;
        armv7)
            if perintah_tersedia arm-linux-gnueabihf-gcc; then
                cc="arm-linux-gnueabihf-gcc"
                as="arm-linux-gnueabihf-as"
                ld="arm-linux-gnueabihf-ld"
            elif perintah_tersedia arm-none-eabi-gcc; then
                cc="arm-none-eabi-gcc"
                as="arm-none-eabi-as"
                ld="arm-none-eabi-ld"
            else
                cc="arm-gcc"
                as="arm-as"
                ld="arm-ld"
            fi
            cflags="-marm -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard"
            ldflags=""
            ;;
        arm64)
            if perintah_tersedia aarch64-linux-gnu-gcc; then
                cc="aarch64-linux-gnu-gcc"
                as="aarch64-linux-gnu-as"
                ld="aarch64-linux-gnu-ld"
            elif perintah_tersedia aarch64-none-elf-gcc; then
                cc="aarch64-none-elf-gcc"
                as="aarch64-none-elf-as"
                ld="aarch64-none-elf-ld"
            else
                cc="aarch64-gcc"
                as="aarch64-as"
                ld="aarch64-ld"
            fi
            cflags="-march=armv8-a"
            ldflags=""
            ;;
        *)
            cetak_error "Arsitektur tidak dikenali: $arsitektur"
            return 1
            ;;
    esac

    echo "$cc|$as|$ld|$cflags|$ldflags"
}

# ========================================
# FUNGSI BUILD
# ========================================

# Buat direktori build
buat_direktori_build() {
    local arsitektur=$1

    cetak_info "Membuat direktori build untuk $arsitektur..."

    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/inti"
    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/arsitektur"
    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/perangkat"
    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/berkas"
    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/libc"
    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/pigura"
    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/dekor"
    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/format"
    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/aplikasi"
    mkdir -p "${DIREKTORI_BUILD}/${arsitektur}/_memuat"
    mkdir -p "${DIREKTORI_OUTPUT}"

    cetak_sukses "Direktori build dibuat"
}

# Kompilasi file C
kompilasi_c() {
    local src=$1
    local obj=$2
    local cc=$3
    local flags=$4
    local arsitektur=$5

    # Flags umum Pigura C90
    local common_flags="-std=c89 -nostdlib -nostdinc -ffreestanding -fno-builtin"
    common_flags="${common_flags} -Wall -Wextra -Werror"
    common_flags="${common_flags} -fno-stack-protector"
    common_flags="${common_flags} -fno-pie -fno-pic"
    common_flags="${common_flags} -Wno-unused-but-set-variable"

    # Definisi arsitektur
    case "$arsitektur" in
        x86)
            common_flags="${common_flags} -DARSITEKTUR_X86=1"
            ;;
        x86_64)
            common_flags="${common_flags} -DARSITEKTUR_X86_64=1"
            ;;
        arm)
            common_flags="${common_flags} -DARSITEKTUR_ARM32=1"
            ;;
        armv7)
            common_flags="${common_flags} -DARSITEKTUR_ARM32=1 -DARMV7=1"
            ;;
        arm64)
            common_flags="${common_flags} -DARSITEKTUR_ARM64=1"
            ;;
    esac

    # Include paths
    local includes="-I${DIREKTORI_SUMBER}/inti"
    includes="${includes} -I${DIREKTORI_SUMBER}/libc/include"

    # Build command
    local cmd="${cc} ${common_flags} ${flags} ${includes} -c \"${src}\" -o \"${obj}\""

    if [ "$VERBOSE" = "1" ]; then
        echo "  Kompilasi: $src"
    fi

    eval "$cmd" 2>/dev/null
    return $?
}

# Kompilasi file Assembly
kompilasi_asm() {
    local src=$1
    local obj=$2
    local as=$3
    local arsitektur=$4

    local flags=""

    case "$arsitektur" in
        x86)
            flags="--32"
            ;;
        x86_64)
            flags="--64"
            ;;
        arm|armv7)
            flags="-marm"
            ;;
        arm64)
            flags=""
            ;;
    esac

    local cmd="${as} ${flags} \"${src}\" -o \"${obj}\""

    if [ "$VERBOSE" = "1" ]; then
        echo "  Assembly: $src"
    fi

    eval "$cmd" 2>/dev/null
    return $?
}

# Link kernel
link_kernel() {
    local arsitektur=$1
    local cc=$2
    local ldflags=$3

    local linker_script="${DIREKTORI_ALAT}/linker_${arsitektur}.ld"
    local output="${DIREKTORI_OUTPUT}/pigura_${arsitektur}.elf"

    cetak_info "Linking kernel..."

    # Kumpulkan semua object files
    local obj_files=$(find "${DIREKTORI_BUILD}/${arsitektur}" -name "*.o" 2>/dev/null)

    if [ -z "$obj_files" ]; then
        cetak_error "Tidak ada file object ditemukan"
        return 1
    fi

    # Link
    local cmd="${cc} -T \"${linker_script}\" -nostdlib -o \"${output}\" ${obj_files}"
    cmd="${cmd} -lgcc"

    if [ -n "$ldflags" ]; then
        cmd="${cc} ${ldflags} -T \"${linker_script}\" -nostdlib -o \"${output}\" ${obj_files}"
    fi

    eval "$cmd" 2>/dev/null

    if [ $? -eq 0 ]; then
        cetak_sukses "Kernel di-link: $output"
        return 0
    else
        cetak_error "Gagal link kernel"
        return 1
    fi
}

# Build kernel untuk arsitektur tertentu
build_kernel() {
    local arsitektur=$1

    cetak_header "BUILD KERNEL ${arsitektur^^}"

    # Dapatkan compiler
    local compiler_info=$(dapatkan_compiler "$arsitektur")
    if [ $? -ne 0 ]; then
        return 1
    fi

    local cc=$(echo "$compiler_info" | cut -d'|' -f1)
    local as=$(echo "$compiler_info" | cut -d'|' -f2)
    local ld=$(echo "$compiler_info" | cut -d'|' -3)
    local cflags=$(echo "$compiler_info" | cut -d'|' -f4)
    local ldflags=$(echo "$compiler_info" | cut -d'|' -f5)

    # Cek compiler
    if ! perintah_tersedia "$cc"; then
        cetak_error "Compiler $cc tidak ditemukan"
        cetak_info "Install cross-compiler untuk $arsitektur"
        return 1
    fi

    cetak_info "Menggunakan compiler: $cc"

    # Buat direktori build
    buat_direktori_build "$arsitektur"

    # Hitung jumlah file
    local jumlah_c=$(find "${DIREKTORI_SUMBER}" -name "*.c" 2>/dev/null | wc -l)
    local jumlah_asm=$(find "${DIREKTORI_SUMBER}" -name "*.S" -o -name "*.s" 2>/dev/null | wc -l)
    local total=$((jumlah_c + jumlah_asm))

    cetak_info "File C: $jumlah_c, Assembly: $jumlah_asm, Total: $total"

    local berhasil=0
    local gagal=0
    local counter=0

    # Kompilasi file C
    cetak_info "Mengkompilasi file C..."

    for src in $(find "${DIREKTORI_SUMBER}" -name "*.c" 2>/dev/null | sort); do
        counter=$((counter + 1))

        # Path relatif untuk output
        local rel_path="${src#${DIREKTORI_SUMBER}/}"
        local obj="${DIREKTORI_BUILD}/${arsitektur}/${rel_path%.c}.o"

        # Buat direktori output
        mkdir -p "$(dirname "$obj")"

        if kompilasi_c "$src" "$obj" "$cc" "$cflags" "$arsitektur"; then
            berhasil=$((berhasil + 1))
        else
            gagal=$((gagal + 1))
            cetak_peringatan "Gagal: $src"
        fi

        # Progress
        if [ $((counter % 50)) -eq 0 ]; then
            cetak_info "Progress: $counter/$total"
        fi
    done

    # Kompilasi file Assembly
    cetak_info "Mengkompilasi file Assembly..."

    for src in $(find "${DIREKTORI_SUMBER}" -name "*.S" -o -name "*.s" 2>/dev/null | sort); do
        counter=$((counter + 1))

        local rel_path="${src#${DIREKTORI_SUMBER}/}"
        local obj="${DIREKTORI_BUILD}/${arsitektur}/${rel_path%.*}.o"

        mkdir -p "$(dirname "$obj")"

        if kompilasi_asm "$src" "$obj" "$as" "$arsitektur"; then
            berhasil=$((berhasil + 1))
        else
            gagal=$((gagal + 1))
            cetak_peringatan "Gagal: $src"
        fi
    done

    echo ""
    cetak_info "Kompilasi selesai: $berhasil berhasil, $gagal gagal"

    if [ $gagal -gt 0 ]; then
        cetak_peringatan "Ada file yang gagal dikompilasi"
    fi

    # Link
    link_kernel "$arsitektur" "$cc" "$ldflags"

    return $?
}

# Build semua arsitektur
build_semua() {
    local arsitektur=("x86" "x86_64" "arm" "armv7" "arm64")
    local berhasil=0
    local gagal=0

    for arch in "${arsitektur[@]}"; do
        if build_kernel "$arch"; then
            berhasil=$((berhasil + 1))
        else
            gagal=$((gagal + 1))
        fi
    done

    echo ""
    cetak_header "RINGKASAN BUILD"
    cetak_info "Arsitektur berhasil: $berhasil"
    cetak_info "Arsitektur gagal: $gagal"
}

# ========================================
# FUNGSI CLEAN
# ========================================

bersihkan() {
    local arsitektur=$1

    cetak_info "Membersihkan build..."

    if [ -n "$arsitektur" ]; then
        rm -rf "${DIREKTORI_BUILD}/${arsitektur}"
        rm -f "${DIREKTORI_OUTPUT}/pigura_${arsitektur}.*"
        cetak_sukses "Build $arsitektur dibersihkan"
    else
        rm -rf "${DIREKTORI_BUILD}"
        rm -rf "${DIREKTORI_OUTPUT}"
        cetak_sukses "Semua build dibersihkan"
    fi
}

# ========================================
# FUNGSI BANTUAN
# ========================================

tampilkan_bantuan() {
    echo ""
    echo "PIGURA OS - Skrip Build"
    echo "======================="
    echo ""
    echo "Penggunaan: $0 [arsitektur] [target] [opsi]"
    echo ""
    echo "Arsitektur:"
    echo "  x86      - Intel/AMD 32-bit"
    echo "  x86_64   - Intel/AMD 64-bit"
    echo "  arm      - ARM 32-bit (legacy)"
    echo "  armv7    - ARMv7 32-bit (modern)"
    echo "  arm64    - ARM64/AArch64 64-bit"
    echo "  all      - Semua arsitektur"
    echo ""
    echo "Target:"
    echo "  kernel   - Build kernel saja (default)"
    echo "  clean    - Bersihkan build"
    echo "  help     - Tampilkan bantuan ini"
    echo ""
    echo "Opsi:"
    echo "  -v, --verbose   Output detail"
    echo "  -j N            Paralel dengan N jobs"
    echo ""
    echo "Contoh:"
    echo "  $0 x86             # Build kernel x86"
    echo "  $0 x86_64 kernel   # Build kernel x86_64"
    echo "  $0 all             # Build semua arsitektur"
    echo "  $0 x86 clean       # Bersihkan build x86"
    echo ""
}

# ========================================
# MAIN
# ========================================

main() {
    # Parse argumen
    local arsitektur=""
    local target="kernel"

    while [ $# -gt 0 ]; do
        case "$1" in
            -v|--verbose)
                VERBOSE=1
                shift
                ;;
            -j)
                JOBS=$2
                shift 2
                ;;
            help|--help|-h)
                tampilkan_bantuan
                exit 0
                ;;
            clean)
                target="clean"
                shift
                ;;
            all)
                arsitektur="all"
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

    # Execute
    case "$target" in
        clean)
            bersihkan "$arsitektur"
            ;;
        kernel)
            if [ "$arsitektur" = "all" ]; then
                build_semua
            else
                build_kernel "$arsitektur"
            fi
            ;;
        *)
            cetak_error "Target tidak dikenali: $target"
            exit 1
            ;;
    esac
}

# Run
main "$@"
