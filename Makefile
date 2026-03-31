# PIGURA OS - MAKEFILE UTAMA
# ==========================
# Makefile untuk membangun kernel Pigura OS.
#
# Penggunaan:
#   make [target] ARCH=[arsitektur]
#
# Target:
#   all       - Build kernel untuk arsitektur default
#   kernel    - Build kernel saja
#   clean     - Bersihkan build
#   image     - Buat disk image
#   run       - Jalankan di QEMU
#   debug     - Debug dengan GDB
#   docs      - Buat dokumentasi
#
# Versi: 1.0
# Tanggal: 2025

# ========================================
# KONFIGURASI
# ========================================

# Include konfigurasi
include Konfigurasi.mk

# Direktori
DIREKTORI_SUMBER    := SUMBER
DIREKTORI_BUILD     := BUILD
DIREKTORI_OUTPUT    := OUTPUT
DIREKTORI_ALAT      := ALAT
DIREKTORI_DOK       := DOKUMENTASI

# Arsip arsitektur yang didukung
ARSITEKTUR_VALID    := x86 x86_64 arm armv7 arm64

# Arsitektur default
ARCH                ?= x86

# ========================================
# COMPILER CONFIGURATION
# ========================================

# Compiler per arsitektur
ifeq ($(ARCH),x86)
    CC              := gcc
    AS              := as
    LD              := ld
    OBJCOPY         := objcopy
    OBJDUMP         := objdump
    CFLAGS          := -m32 -march=i686
    ASFLAGS         := --32
    LDFLAGS         := -Wl,-melf_i386
    QEMU            := qemu-system-i386
    LINKER_SCRIPT   := $(DIREKTORI_ALAT)/linker_x86.ld
else ifeq ($(ARCH),x86_64)
    CC              := gcc
    AS              := as
    LD              := ld
    OBJCOPY         := objcopy
    OBJDUMP         := objdump
    CFLAGS          := -m64 -march=x86-64
    ASFLAGS         := --64
    LDFLAGS         := -Wl,-melf_x86_64
    QEMU            := qemu-system-x86_64
    LINKER_SCRIPT   := $(DIREKTORI_ALAT)/linker_x86_64.ld
else ifeq ($(ARCH),arm)
    CC              := arm-linux-gnueabi-gcc
    AS              := arm-linux-gnueabi-as
    LD              := arm-linux-gnueabi-ld
    OBJCOPY         := arm-linux-gnueabi-objcopy
    OBJDUMP         := arm-linux-gnueabi-objdump
    CFLAGS          := -marm -march=armv6 -mfloat-abi=soft
    ASFLAGS         :=
    LDFLAGS         :=
    QEMU            := qemu-system-arm
    LINKER_SCRIPT   := $(DIREKTORI_ALAT)/linker_arm.ld
else ifeq ($(ARCH),armv7)
    CC              := arm-linux-gnueabihf-gcc
    AS              := arm-linux-gnueabihf-as
    LD              := arm-linux-gnueabihf-ld
    OBJCOPY         := arm-linux-gnueabihf-objcopy
    OBJDUMP         := arm-linux-gnueabihf-objdump
    CFLAGS          := -marm -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=hard
    ASFLAGS         :=
    LDFLAGS         :=
    QEMU            := qemu-system-arm
    LINKER_SCRIPT   := $(DIREKTORI_ALAT)/linker_armv7.ld
else ifeq ($(ARCH),arm64)
    CC              := aarch64-linux-gnu-gcc
    AS              := aarch64-linux-gnu-as
    LD              := aarch64-linux-gnu-ld
    OBJCOPY         := aarch64-linux-gnu-objcopy
    OBJDUMP         := aarch64-linux-gnu-objdump
    CFLAGS          := -march=armv8-a
    ASFLAGS         :=
    LDFLAGS         :=
    QEMU            := qemu-system-aarch64
    LINKER_SCRIPT   := $(DIREKTORI_ALAT)/linker_arm64.ld
endif

# Flags umum Pigura C90
CFLAGS_COMMON       := -std=c89 -nostdlib -nostdinc -ffreestanding
CFLAGS_COMMON       += -fno-builtin -fno-stack-protector
CFLAGS_COMMON       += -fno-pie -fno-pic
CFLAGS_COMMON       += -Wall -Wextra -Werror
CFLAGS_COMMON       += -Wno-unused-but-set-variable
CFLAGS_COMMON       += -Wno-unused-function

# Debug flags
ifdef DEBUG
    CFLAGS_COMMON   += -g -O0 -DDEBUG=1
else
    CFLAGS_COMMON   += -O2 -DNDEBUG
endif

# Include paths
INCLUDES            := -I$(DIREKTORI_SUMBER)/inti
INCLUDES            += -I$(DIREKTORI_SUMBER)/libc/include

# Output files
KERNEL_ELF          := $(DIREKTORI_OUTPUT)/pigura_$(ARCH).elf
KERNEL_BIN          := $(DIREKTORI_OUTPUT)/pigura_$(ARCH).bin
KERNEL_MAP          := $(DIREKTORI_OUTPUT)/pigura_$(ARCH).map

# ========================================
# SOURCE FILES
# ========================================

# Cari semua file sumber (exclude arsitektur lain)
_OTHER_ARCH := $(filter-out $(ARCH),x86 x86_64 arm armv7 arm64)
_SKIP_C     := $(foreach a,$(_OTHER_ARCH),-not -path '*/arsitektur/$(a)/*')
SKIP_LIBC   := $(foreach a,$(_OTHER_ARCH),-not -path '*/libc/internal/arch/$(a)/*')
SKIP_ASM    := $(foreach a,$(_OTHER_ARCH),-not -path '*/arsitektur/$(a)/*')
SUMBER_C   := $(shell find $(DIREKTORI_SUMBER) $(_SKIP_C) $(SKIP_LIBC) -name '*.c' 2>/dev/null)
SUMBER_ASM := $(shell find $(DIREKTORI_SUMBER) $(SKIP_ASM) $(SKIP_LIBC) \( -name '*.S' -o -name '*.s' \) 2>/dev/null)

# Object files
OBJEK_C             := $(patsubst $(DIREKTORI_SUMBER)/%.c,$(DIREKTORI_BUILD)/$(ARCH)/%.o,$(SUMBER_C))
OBJEK_ASM           := $(patsubst $(DIREKTORI_SUMBER)/%.S,$(DIREKTORI_BUILD)/$(ARCH)/%.o,$(SUMBER_ASM))
OBJEK_ASM           += $(patsubst $(DIREKTORI_SUMBER)/%.s,$(DIREKTORI_BUILD)/$(ARCH)/%.o,$(SUMBER_ASM))
OBJEK               := $(OBJEK_C) $(OBJEK_ASM)

# ========================================
# PHONY TARGETS
# ========================================

.PHONY: all kernel clean clean-all image run debug docs help
.PHONY: info check-deps

# ========================================
# TARGET UTAMA
# ========================================

# Default target
all: check-deps kernel

# Build kernel
kernel: direktori $(KERNEL_ELF)

# ========================================
# BUILD RULES
# ========================================

# Buat direktori build
direktori:
	@echo "[INFO] Membuat direktori build..."
	@mkdir -p $(DIREKTORI_BUILD)/$(ARCH)
	@mkdir -p $(DIREKTORI_OUTPUT)
	@mkdir -p $(dir $(OBJEK))

# Link kernel
$(KERNEL_ELF): $(OBJEK)
	@echo "[INFO] Linking kernel..."
	@$(CC) $(LDFLAGS) -T $(LINKER_SCRIPT) -nostdlib \
	-o $@ $(OBJEK) -lgcc \
	-Wl,-Map=$(KERNEL_MAP)
	@echo "[SUKSES] Kernel: $@"
	@$(OBJDUMP) -h $@ > $(DIREKTORI_OUTPUT)/sections_$(ARCH).txt
	@$(OBJDUMP) -t $@ > $(DIREKTORI_OUTPUT)/symbols_$(ARCH).txt

# Kompilasi file C
$(DIREKTORI_BUILD)/$(ARCH)/%.o: $(DIREKTORI_SUMBER)/%.c
	@mkdir -p $(dir $@)
	@echo "  CC      $<"
	@$(CC) $(CFLAGS_COMMON) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Kompilasi file Assembly
$(DIREKTORI_BUILD)/$(ARCH)/%.o: $(DIREKTORI_SUMBER)/%.S
	@mkdir -p $(dir $@)
	@echo "  AS      $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(DIREKTORI_BUILD)/$(ARCH)/%.o: $(DIREKTORI_SUMBER)/%.s
	@mkdir -p $(dir $@)
	@echo "  AS      $<"
	@$(AS) $(ASFLAGS) $< -o $@

# Buat binary mentah
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "[INFO] Creating binary..."
	@$(OBJCOPY) -O binary $< $@
	@echo "[SUKSES] Binary: $@"

# ========================================
# TARGET TAMBAHAN
# ========================================

# Buat disk image
image: kernel
	@echo "[INFO] Creating disk image..."
	@$(DIREKTORI_ALAT)/create_image.sh $(ARCH) raw

# Jalankan di QEMU
run: kernel
	@echo "[INFO] Running in QEMU..."
	@$(DIREKTORI_ALAT)/qemu_run.sh $(ARCH) text

# Debug dengan GDB
debug: kernel
	@echo "[INFO] Starting debug session..."
	@$(DIREKTORI_ALAT)/debug.sh $(ARCH) server

# Buat dokumentasi
docs:
	@echo "[INFO] Building documentation..."
	@if [ -d "$(DIREKTORI_DOK)" ]; then \
	echo "[INFO] Documentation available in $(DIREKTORI_DOK)"; \
	else \
	echo "[WARNING] No documentation found"; \
	fi

# ========================================
# CLEAN TARGETS
# ========================================

# Bersihkan build arsitektur ini
clean:
	@echo "[INFO] Cleaning build for $(ARCH)..."
	@rm -rf $(DIREKTORI_BUILD)/$(ARCH)
	@rm -f $(DIREKTORI_OUTPUT)/pigura_$(ARCH).*
	@rm -f $(DIREKTORI_OUTPUT)/*_$(ARCH).*
	@echo "[SUKSES] Cleaned"

# Bersihkan semua
clean-all:
	@echo "[INFO] Cleaning all builds..."
	@rm -rf $(DIREKTORI_BUILD)
	@rm -rf $(DIREKTORI_OUTPUT)
	@rm -f *.img *.iso *.log
	@echo "[SUKSES] All cleaned"

# ========================================
# INFO TARGETS
# ========================================

# Tampilkan informasi
info:
	@echo ""
	@echo "======================================"
	@echo " PIGURA OS Build Info"
	@echo "======================================"
	@echo ""
	@echo "  Nama:           $(PIGURA_NAMA)"
	@echo "  Versi:          $(PIGURA_VERSI)"
	@echo "  Arsitektur:     $(ARCH)"
	@echo "  Compiler:       $(CC)"
	@echo "  Linker Script:  $(LINKER_SCRIPT)"
	@echo ""
	@echo "  File C:         $(words $(SUMBER_C))"
	@echo "  File Assembly:  $(words $(SUMBER_ASM))"
	@echo "  Total Objects:  $(words $(OBJEK))"
	@echo ""
	@echo "======================================"
	@echo ""

# Cek dependencies
check-deps:
	@echo "[INFO] Checking dependencies..."
	@if ! command -v $(CC) >/dev/null 2>&1; then \
	echo "[ERROR] Compiler $(CC) tidak ditemukan"; \
	exit 1; \
	fi
	@if ! command -v $(AS) >/dev/null 2>&1; then \
	echo "[ERROR] Assembler $(AS) tidak ditemukan"; \
	exit 1; \
	fi
	@if [ ! -f $(LINKER_SCRIPT) ]; then \
	echo "[ERROR] Linker script tidak ditemukan: $(LINKER_SCRIPT)"; \
	exit 1; \
	fi
	@echo "[SUKSES] Dependencies OK"

# ========================================
# HELP
# ========================================

help:
	@echo ""
	@echo "PIGURA OS - Makefile"
	@echo "===================="
	@echo ""
	@echo "Penggunaan: make [target] ARCH=[arsitektur] [opsi]"
	@echo ""
	@echo "Target:"
	@echo "  all         - Build kernel (default)"
	@echo "  kernel      - Build kernel saja"
	@echo "  clean       - Bersihkan build arsitektur ini"
	@echo "  clean-all   - Bersihkan semua build"
	@echo "  image       - Buat disk image"
	@echo "  run         - Jalankan di QEMU"
	@echo "  debug       - Debug dengan GDB"
	@echo "  docs        - Buat dokumentasi"
	@echo "  info        - Tampilkan informasi build"
	@echo "  check-deps  - Cek dependencies"
	@echo "  help        - Tampilkan bantuan ini"
	@echo ""
	@echo "Arsitektur:"
	@echo "  x86         - Intel/AMD 32-bit"
	@echo "  x86_64      - Intel/AMD 64-bit"
	@echo "  arm         - ARM 32-bit (legacy)"
	@echo "  armv7       - ARMv7 32-bit"
	@echo "  arm64       - ARM64 64-bit"
	@echo ""
	@echo "Opsi:"
	@echo "  DEBUG=1     - Build dengan debug symbols"
	@echo "  V=1         - Verbose output"
	@echo ""
	@echo "Contoh:"
	@echo "  make ARCH=x86               # Build x86"
	@echo "  make ARCH=x86_64 DEBUG=1    # Debug build x86_64"
	@echo "  make ARCH=arm64 run         # Build dan run arm64"
	@echo ""
