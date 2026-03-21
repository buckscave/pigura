#!/bin/bash
#
# PIGURA OS - SKRIP PEMBERSIHAN
# -----------------------------
# Skrip ini membersihkan semua artefak build untuk
# Pigura OS.
#
# Penggunaan: ./clean.sh [arsitektur]
#   arsitektur: x86, x86_64, arm, armv7, arm64, all
#
# Versi: 1.0
# Tanggal: 2025

# ========================================
# KONFIGURASI
# ========================================

DIREKTORI_AKAR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIREKTORI_BUILD="${DIREKTORI_AKAR}/BUILD"
DIREKTORI_OUTPUT="${DIREKTORI_AKAR}/OUTPUT"

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

# ========================================
# FUNGSI PEMBERSIHAN
# ========================================

# Bersihkan arsitektur tertentu
bersihkan_arsitektur() {
    local arsitektur=$1

    cetak_info "Membersihkan arsitektur: $arsitektur"

    # Hapus direktori build
    if [ -d "${DIREKTORI_BUILD}/${arsitektur}" ]; then
        rm -rf "${DIREKTORI_BUILD}/${arsitektur}"
        cetak_sukses "Direktori build $arsitektur dihapus"
    else
        cetak_peringatan "Direktori build $arsitektur tidak ditemukan"
    fi

    # Hapus output files
    local files=$(find "${DIREKTORI_OUTPUT}" -name "pigura_${arsitektur}.*" 2>/dev/null)
    if [ -n "$files" ]; then
        rm -f $files
        cetak_sukses "File output $arsitektur dihapus"
    fi

    # Hapus object files
    find "${DIREKTORI_AKAR}" -name "*.o" -path "*/${arsitektur}/*" -delete 2>/dev/null

    # Hapus dependency files
    find "${DIREKTORI_AKAR}" -name "*.d" -path "*/${arsitektur}/*" -delete 2>/dev/null
}

# Bersihkan semua
bersihkan_semua() {
    cetak_info "Membersihkan semua build..."

    # Hapus direktori build
    if [ -d "${DIREKTORI_BUILD}" ]; then
        rm -rf "${DIREKTORI_BUILD}"
        cetak_sukses "Direktori BUILD dihapus"
    fi

    # Hapus direktori output
    if [ -d "${DIREKTORI_OUTPUT}" ]; then
        rm -rf "${DIREKTORI_OUTPUT}"
        cetak_sukses "Direktori OUTPUT dihapus"
    fi

    # Hapus semua object files
    find "${DIREKTORI_AKAR}" -name "*.o" -delete 2>/dev/null
    find "${DIREKTORI_AKAR}" -name "*.d" -delete 2>/dev/null

    # Hapus backup files
    find "${DIREKTORI_AKAR}" -name "*~" -delete 2>/dev/null
    find "${DIREKTORI_AKAR}" -name "*.bak" -delete 2>/dev/null

    # Hapus core dumps
    find "${DIREKTORI_AKAR}" -name "core" -delete 2>/dev/null
    find "${DIREKTORI_AKAR}" -name "core.*" -delete 2>/dev/null

    cetak_sukses "Semua build dibersihkan"
}

# Bersihkan disk images
bersihkan_image() {
    cetak_info "Membersihkan disk images..."

    local images=$(find "${DIREKTORI_AKAR}" -name "*.img" -o -name "*.iso" 2>/dev/null)

    if [ -n "$images" ]; then
        rm -f $images
        cetak_sukses "Disk images dihapus"
    else
        cetak_peringatan "Tidak ada disk images ditemukan"
    fi
}

# Bersihkan log files
bersihkan_log() {
    cetak_info "Membersihkan log files..."

    find "${DIREKTORI_AKAR}" -name "*.log" -delete 2>/dev/null

    cetak_sukses "Log files dihapus"
}

# Tampilkan statistik sebelum/akhir
tampilkan_statistik() {
    local total_size=0

    echo ""
    echo "======================================"
    echo " STATISTIK DIREKTORI"
    echo "======================================"

    if [ -d "${DIREKTORI_BUILD}" ]; then
        local build_size=$(du -sh "${DIREKTORI_BUILD}" 2>/dev/null | cut -f1)
        echo "  BUILD:  $build_size"
    else
        echo "  BUILD:  (tidak ada)"
    fi

    if [ -d "${DIREKTORI_OUTPUT}" ]; then
        local output_size=$(du -sh "${DIREKTORI_OUTPUT}" 2>/dev/null | cut -f1)
        echo "  OUTPUT: $output_size"
    else
        echo "  OUTPUT: (tidak ada)"
    fi

    echo "======================================"
    echo ""
}

# ========================================
# BANTUAN
# ========================================

tampilkan_bantuan() {
    echo ""
    echo "PIGURA OS - Skrip Pembersihan"
    echo "=============================="
    echo ""
    echo "Penggunaan: $0 [opsi] [arsitektur]"
    echo ""
    echo "Opsi:"
    echo "  -a, --all       Bersihkan semua arsitektur"
    echo "  -i, --images    Bersihkan disk images juga"
    echo "  -l, --logs      Bersihkan log files juga"
    echo "  -s, --stats     Tampilkan statistik"
    echo "  -h, --help      Tampilkan bantuan ini"
    echo ""
    echo "Arsitektur:"
    echo "  x86      - Intel/AMD 32-bit"
    echo "  x86_64   - Intel/AMD 64-bit"
    echo "  arm      - ARM 32-bit"
    echo "  armv7    - ARMv7 32-bit"
    echo "  arm64    - ARM64 64-bit"
    echo ""
    echo "Contoh:"
    echo "  $0 x86          # Bersihkan build x86"
    echo "  $0 --all        # Bersihkan semua build"
    echo "  $0 -a -i        # Bersihkan semua + disk images"
    echo ""
}

# ========================================
# MAIN
# ========================================

main() {
    local arsitektur=""
    local bersihkan_semua_flag=0
    local bersihkan_images=0
    local bersihkan_logs=0
    local tampilkan_stats=0

    # Parse argumen
    while [ $# -gt 0 ]; do
        case "$1" in
            -a|--all)
                bersihkan_semua_flag=1
                shift
                ;;
            -i|--images)
                bersihkan_images=1
                shift
                ;;
            -l|--logs)
                bersihkan_logs=1
                shift
                ;;
            -s|--stats)
                tampilkan_stats=1
                shift
                ;;
            -h|--help|help)
                tampilkan_bantuan
                exit 0
                ;;
            x86|x86_64|arm|armv7|arm64)
                arsitektur=$1
                shift
                ;;
            *)
                echo "Argumen tidak dikenali: $1"
                tampilkan_bantuan
                exit 1
                ;;
        esac
    done

    # Tampilkan statistik sebelum
    if [ $tampilkan_stats -eq 1 ]; then
        echo ""
        echo "SEBELUM:"
        tampilkan_statistik
    fi

    # Execute
    if [ $bersihkan_semua_flag -eq 1 ]; then
        bersihkan_semua
    elif [ -n "$arsitektur" ]; then
        bersihkan_arsitektur "$arsitektur"
    else
        # Default: bersihkan semua
        bersihkan_semua
    fi

    # Tambahan
    if [ $bersihkan_images -eq 1 ]; then
        bersihkan_image
    fi

    if [ $bersihkan_logs -eq 1 ]; then
        bersihkan_log
    fi

    # Tampilkan statistik sesudah
    if [ $tampilkan_stats -eq 1 ]; then
        echo ""
        echo "SESUDAH:"
        tampilkan_statistik
    fi

    cetak_sukses "Pembersihan selesai"
}

# Run
main "$@"
