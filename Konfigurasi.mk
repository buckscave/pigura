# PIGURA OS - KONFIGURASI.MK
# =========================
# File konfigurasi build untuk Pigura OS.
#
# File ini berisi semua konfigurasi yang dapat diubah
# untuk menyesuaikan proses build.
#
# Versi: 1.0
# Tanggal: 2025

# ========================================
# INFORMASI KERNEL
# ========================================

# Nama kernel
PIGURA_NAMA         := Pigura OS

# Versi kernel
PIGURA_VERSI_MAJOR  := 1
PIGURA_VERSI_MINOR  := 0
PIGURA_VERSI_PATCH  := 0
PIGURA_VERSI        := $(PIGURA_VERSI_MAJOR).$(PIGURA_VERSI_MINOR).$(PIGURA_VERSI_PATCH)

# Julukan
PIGURA_JULUKAN      := Bingkai Digital

# ========================================
# KONFIGURASI ARSITEKTUR
# ========================================

# Arsitektur default (dapat di-override dengan ARCH=)
ARCH_DEFAULT        := x86

# Arsitektur yang didukung
ARCH_X86            := x86
ARCH_X86_64         := x86_64
ARCH_ARM            := arm
ARCH_ARMV7          := armv7
ARCH_ARM64          := arm64

# ========================================
# KONFIGURASI BUILD
# ========================================

# Compiler flags tambahan
EXTRA_CFLAGS        :=

# Assembler flags tambahan
EXTRA_ASFLAGS       :=

# Linker flags tambahan
EXTRA_LDFLAGS       :=

# Optimization level (0-3, s, z)
OPT_LEVEL           := 2

# Debug build (0 atau 1)
DEBUG_BUILD         := 0

# Verbose output (0 atau 1)
VERBOSE             := 0

# ========================================
# KONFIGURASI MEMORI
# ========================================

# Ukuran memori QEMU (dalam MB)
QEMU_MEMORY         := 512

# Ukuran stack kernel (dalam KB)
KERNEL_STACK_SIZE   := 64

# Ukuran heap awal (dalam KB)
KERNEL_HEAP_SIZE    := 1024

# ========================================
# KONFIGURASI STORAGE
# ========================================

# Ukuran disk image default (dalam MB)
DISK_IMAGE_SIZE     := 64

# Nama file disk image
DISK_IMAGE_NAME     := pigura

# ========================================
# KONFIGURASI QEMU
# ========================================

# Machine type per arsitektur
QEMU_MACHINE_x86    := q35
QEMU_MACHINE_x86_64 := q35
QEMU_MACHINE_arm    := versatilepb
QEMU_MACHINE_armv7  := virt
QEMU_MACHINE_arm64  := virt

# CPU type per arsitektur
QEMU_CPU_x86        := qemu32,+pae
QEMU_CPU_x86_64     := qemu64
QEMU_CPU_arm        := arm926
QEMU_CPU_armv7      := cortex-a15
QEMU_CPU_arm64      := cortex-a57

# Port GDB
GDB_PORT            := 1234

# ========================================
# KONFIGURASI OUTPUT
# ========================================

# Warna output
WARNA_AKTIF         := 1

# Level log (0-4)
# 0 = tidak ada
# 1 = error
# 2 = warning
# 3 = info
# 4 = debug
LOG_LEVEL           := 3

# File log
LOG_FILE            := build.log

# ========================================
# KONFIGURASI PATH
# ========================================

# Direktori utama
DIREKTORI_AKAR      := $(dir $(lastword $(MAKEFILE_LIST)))

# Direktori sumber
DIREKTORI_SUMBER    := $(DIREKTORI_AKAR)SUMBER

# Direktori build
DIREKTORI_BUILD     := $(DIREKTORI_AKAR)BUILD

# Direktori output
DIREKTORI_OUTPUT    := $(DIREKTORI_AKAR)OUTPUT

# Direktori alat
DIREKTORI_ALAT      := $(DIREKTORI_AKAR)ALAT

# Direktori dokumentasi
DIREKTORI_DOK       := $(DIREKTORI_AKAR)DOKUMENTASI

# Direktori image
DIREKTORI_IMAGE     := $(DIREKTORI_AKAR)IMAGE

# Direktori ISO
DIREKTORI_ISO       := $(DIREKTORI_AKAR)ISO

# ========================================
# KONFIGURASI CROSS-COMPILER
# ========================================

# Prefix cross-compiler ARM
CROSS_ARM_PREFIX    := arm-linux-gnueabi-

# Prefix cross-compiler ARM hard-float
CROSS_ARMHF_PREFIX  := arm-linux-gnueabihf-

# Prefix cross-compiler ARM bare-metal
CROSS_ARM_NONE      := arm-none-eabi-

# Prefix cross-compiler AArch64
CROSS_AARCH64_PREFIX := aarch64-linux-gnu-

# Prefix cross-compiler AArch64 bare-metal
CROSS_AARCH64_NONE  := aarch64-none-elf-

# ========================================
# KONFIGURASI BOOTLOADER
# ========================================

# Bootloader (grub, syslinux, limine)
BOOTLOADER          := grub

# Multiboot version (1 atau 2)
MULTIBOOT_VERSION   := 1

# Boot timeout (detik)
BOOT_TIMEOUT        := 5

# ========================================
# KONFIGURASI FILESYSTEM
# ========================================

# Filesystem untuk disk image (fat32, ext2, pigurafs)
FILESYSTEM_TYPE     := fat32

# Label volume
VOLUME_LABEL        := PIGURA

# ========================================
# KONFIGURASI TESTING
# ========================================

# Jalankan test setelah build
RUN_TESTS           := 0

# Direktori test
TEST_DIR            := $(DIREKTORI_AKAR)TEST

# ========================================
# KONFIGURASI INSTALASI
# ========================================

# Prefix instalasi
INSTALL_PREFIX      := /usr/local

# Direktori instalasi kernel
INSTALL_KERNEL      := $(INSTALL_PREFIX)/boot

# Direktori instalasi modul
INSTALL_MODULES     := $(INSTALL_PREFIX)/lib/modules

# ========================================
# DETEKSI OTOMATIS
# ========================================

# Deteksi OS host
UNAME_S             := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    HOST_OS         := linux
else ifeq ($(UNAME_S),Darwin)
    HOST_OS         := macos
else
    HOST_OS         := unknown
endif

# Deteksi arsitektur host
UNAME_M             := $(shell uname -m)
ifeq ($(UNAME_M),x86_64)
    HOST_ARCH       := x86_64
else ifeq ($(UNAME_M),i686)
    HOST_ARCH       := x86
else ifeq ($(UNAME_M),aarch64)
    HOST_ARCH       := arm64
else ifeq ($(UNAME_M),armv7l)
    HOST_ARCH       := armv7
else
    HOST_ARCH       := unknown
endif

# ========================================
# HELPER FUNCTIONS
# ========================================

# Fungsi untuk mendapatkan nilai QEMU machine
define get_qemu_machine
$(if $(filter $(ARCH),x86),$(QEMU_MACHINE_x86),
$(if $(filter $(ARCH),x86_64),$(QEMU_MACHINE_x86_64),
$(if $(filter $(ARCH),arm),$(QEMU_MACHINE_arm),
$(if $(filter $(ARCH),armv7),$(QEMU_MACHINE_armv7),
$(if $(filter $(ARCH),arm64),$(QEMU_MACHINE_arm64),
unknown)))))
endef

# Fungsi untuk mendapatkan nilai QEMU CPU
define get_qemu_cpu
$(if $(filter $(ARCH),x86),$(QEMU_CPU_x86),
$(if $(filter $(ARCH),x86_64),$(QEMU_CPU_x86_64),
$(if $(filter $(ARCH),arm),$(QEMU_CPU_arm),
$(if $(filter $(ARCH),armv7),$(QEMU_CPU_armv7),
$(if $(filter $(ARCH),arm64),$(QEMU_CPU_arm64),
unknown)))))
endef

# ========================================
# VALIDASI
# ========================================

# Validasi arsitektur
VALID_ARCH          := $(filter $(ARCH),$(ARCH_X86) $(ARCH_X86_64) $(ARCH_ARM) $(ARCH_ARMV7) $(ARCH_ARM64))
ifeq ($(VALID_ARCH),)
    $(error Arsitektur tidak valid: $(ARCH). Gunakan: x86, x86_64, arm, armv7, atau arm64)
endif

# ========================================
# EXPORT
# ========================================

export PIGURA_NAMA
export PIGURA_VERSI
export ARCH
export DEBUG_BUILD
export VERBOSE
export QEMU_MEMORY
