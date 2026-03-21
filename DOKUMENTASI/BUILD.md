# PANDUAN BUILD PIGURA OS

## Daftar Isi

1. [Persyaratan Sistem](#persyaratan-sistem)
2. [Instalasi Dependensi](#instalasi-dependensi)
3. [Konfigurasi Build](#konfigurasi-build)
4. [Perintah Build](#perintah-build)
5. [Build Multi-Arsitektur](#build-multi-arsitektur)
6. [Menjalankan di QEMU](#menjalankan-di-qemu)
7. [Debugging](#debugging)
8. [Membuat Disk Image](#membuat-disk-image)
9. [Troubleshooting](#troubleshooting)

---

## Persyaratan Sistem

### Sistem Operasi Host

Pigura OS dapat dibangun di sistem operasi berikut:

| OS Host | Status | Keterangan |
|---------|--------|------------|
| Linux (Ubuntu/Debian) | Disarankan | Toolchain lengkap tersedia |
| Linux (Arch/Manjaro) | Didukung | Install package yang sesuai |
| Linux (Fedora/RHEL) | Didukung | Install package yang sesuai |
| macOS | Didukung | Perlu toolchain cross-compile |
| Windows (WSL2) | Didukung | Via Windows Subsystem for Linux |
| Windows (Native) | Terbatas | Perlu MSYS2 atau Cygwin |

### Hardware Minimum

| Komponen | Minimum | Disarankan |
|----------|---------|------------|
| CPU | 2 core | 4+ core |
| RAM | 2 GB | 4+ GB |
| Storage | 5 GB | 10+ GB |

---

## Instalasi Dependensi

### Ubuntu/Debian

```bash
# Compiler dan toolchain dasar
sudo apt update
sudo apt install build-essential gcc g++ make

# Cross-compiler untuk berbagai arsitektur
sudo apt install gcc-multilib g++-multilib           # x86 32-bit
sudo apt install gcc-arm-linux-gnueabi               # ARM
sudo apt install gcc-arm-linux-gnueabihf             # ARM hard-float
sudo apt install gcc-aarch64-linux-gnu               # ARM64

# Emulator
sudo apt install qemu-system-x86 qemu-system-arm

# Tools tambahan
sudo apt install nasm xorriso mtools grub-pc-bin grub-common

# Debugging (opsional)
sudo apt install gdb-multiarch

# Documentation (opsional)
sudo apt install doxygen graphviz
```

### Arch Linux/Manjaro

```bash
# Compiler dan toolchain
sudo pacman -S base-devel gcc make

# Cross-compiler
sudo pacman -S arm-linux-gnueabihf-gcc aarch64-linux-gnu-gcc

# Emulator
sudo pacman -S qemu-headless

# Tools tambahan
sudo pacman -S nasm xorriso mtools grub

# Debugging
sudo pacman -S gdb
```

### Fedora/RHEL

```bash
# Compiler dan toolchain
sudo dnf install gcc gcc-c++ make

# Cross-compiler
sudo dnf install arm-linux-gnueabihf-gcc aarch64-linux-gnu-gcc

# Emulator
sudo dnf install qemu-system-x86 qemu-system-arm

# Tools tambahan
sudo dnf install nasm xorriso mtools grub2-tools
```

### macOS

```bash
# Install Homebrew jika belum
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Compiler dan toolchain
brew install gcc make

# Cross-compiler
brew install arm-linux-gnueabihf-binutils aarch64-linux-gnu-binutils

# Emulator
brew install qemu

# Tools tambahan
brew install nasm xorriso mtools
```

---

## Konfigurasi Build

### File Konfigurasi.mk

File `Konfigurasi.mk` berisi semua pengaturan build yang dapat dikustomisasi:

```makefile
# Versi kernel
PIGURA_VERSI_MAJOR  := 1
PIGURA_VERSI_MINOR  := 0
PIGURA_VERSI_PATCH  := 0

# Arsitektur default
ARCH_DEFAULT        := x86

# Memory QEMU (MB)
QEMU_MEMORY         := 512

# Ukuran disk image (MB)
DISK_IMAGE_SIZE     := 64

# Bootloader (grub, syslinux)
BOOTLOADER          := grub

# Filesystem untuk image
FILESYSTEM_TYPE     := fat32
```

### Variabel Build

Variabel dapat di-set saat menjalankan make:

| Variabel | Default | Deskripsi |
|----------|---------|-----------|
| `ARCH` | x86 | Arsitektur target |
| `DEBUG` | 0 | Build dengan debug symbols |
| `VERBOSE` | 0 | Output detail |

---

## Perintah Build

### Menggunakan Makefile

```bash
# Build kernel untuk arsitektur default (x86)
make

# Build untuk arsitektur tertentu
make ARCH=x86
make ARCH=x86_64
make ARCH=arm
make ARCH=armv7
make ARCH=arm64

# Build dengan debug symbols
make ARCH=x86_64 DEBUG=1

# Build dengan output verbose
make ARCH=x86 VERBOSE=1

# Tampilkan informasi build
make info

# Bersihkan build arsitektur ini
make clean

# Bersihkan semua build
make clean-all
```

### Menggunakan Skrip build.sh

```bash
# Build kernel x86
./ALAT/build.sh x86

# Build kernel x86_64
./ALAT/build.sh x86_64

# Build semua arsitektur
./ALAT/build.sh all

# Tampilkan bantuan
./ALAT/build.sh help

# Build dengan output verbose
./ALAT/build.sh x86 -v
```

### Output Build

Hasil build akan ditempatkan di direktori berikut:

```
PIGURA/
├── BUILD/
│   └── x86/                    # Object files per arsitektur
│       └── *.o
│
└── OUTPUT/
    ├── pigura_x86.elf          # Kernel ELF
    ├── pigura_x86.bin          # Kernel binary mentah
    ├── pigura_x86.map          # Linker map
    ├── sections_x86.txt        # Section headers
    └── symbols_x86.txt         # Symbol table
```

---

## Build Multi-Arsitektur

### Build Semua Arsitektur Sekaligus

```bash
# Menggunakan skrip
./ALAT/build.sh all

# Atau manual
make ARCH=x86 && make ARCH=x86_64 && make ARCH=arm && make ARCH=armv7 && make ARCH=arm64
```

### Perbandingan Compiler Per Arsitektur

| Arsitektur | Compiler | CFLAGS |
|------------|----------|--------|
| x86 | gcc -m32 | `-m32 -march=i686` |
| x86_64 | gcc -m64 | `-m64 -march=x86-64` |
| arm | arm-linux-gnueabi-gcc | `-marm -march=armv6` |
| armv7 | arm-linux-gnueabihf-gcc | `-marm -march=armv7-a` |
| arm64 | aarch64-linux-gnu-gcc | `-march=armv8-a` |

### Linker Scripts

Setiap arsitektur memiliki linker script sendiri:

```
ALAT/
├── linker_x86.ld       # x86 32-bit
├── linker_x86_64.ld    # x86_64 64-bit
├── linker_arm.ld       # ARM legacy
├── linker_armv7.ld     # ARMv7
└── linker_arm64.ld     # ARM64/AArch64
```

---

## Menjalankan di QEMU

### Mode Teks (Serial Console)

```bash
# Menggunakan skrip
./ALAT/qemu_run.sh x86 text

# Atau manual
qemu-system-i386 -kernel OUTPUT/pigura_x86.elf -nographic -serial mon:stdio

# x86_64
./ALAT/qemu_run.sh x86_64 text

# ARM64
./ALAT/qemu_run.sh arm64 text -m 1G
```

### Mode Grafis

```bash
# Dengan display
./ALAT/qemu_run.sh x86_64 graphic

# Atau manual
qemu-system-x86_64 -kernel OUTPUT/pigura_x86_64.elf -vga std -m 512M
```

### Boot dari Disk Image

```bash
# Buat image terlebih dahulu
./ALAT/create_image.sh x86 iso

# Jalankan dari image
./ALAT/qemu_run.sh x86 image -i IMAGE/pigura_x86.img
```

### Keyboard Shortcuts di QEMU

| Shortcut | Fungsi |
|----------|--------|
| `Ctrl+A X` | Keluar dari QEMU |
| `Ctrl+A C` | Toggle console/monitor |
| `Ctrl+A S` | Screenshot |
| `Ctrl+A D` | Detach (jika berjalan) |

---

## Debugging

### Menjalankan Debug Server

```bash
# Terminal 1: Jalankan QEMU dengan GDB server
./ALAT/debug.sh x86 server

# Atau manual
qemu-system-i386 -kernel OUTPUT/pigura_x86.elf -s -S -nographic
```

### Menjalankan GDB Client

```bash
# Terminal 2: Jalankan GDB
./ALAT/debug.sh x86 gdb

# Atau manual
gdb-multiarch OUTPUT/pigura_x86.elf
(gdb) target remote localhost:1234
```

### Perintah GDB Berguna

```gdb
# Breakpoints
break _start
break kernel_main
break panic

# Execution
continue          # Lanjut eksekusi
stepi             # Step instruksi
nexti             # Next instruksi

# Inspection
info registers    # Lihat register
x/10i $pc         # Disassemble dari PC
bt                # Backtrace
info breakpoints  # Daftar breakpoint

# Memory
x/10x 0xC0000000  # Dump memory hex
x/10i 0x100000    # Disassemble alamat
```

### Dump Symbols dan Sections

```bash
# Lihat symbol table
./ALAT/debug.sh x86 symbols

# Lihat section headers
./ALAT/debug.sh x86 sections
```

---

## Membuat Disk Image

### Tipe Image yang Tersedia

| Tipe | Deskripsi | Ukuran |
|------|-----------|--------|
| raw | Raw disk image | Default 64MB |
| iso | Bootable ISO | ~2MB |
| vdi | VirtualBox image | Variable |
| qcow2 | QEMU image | Variable |

### Membuat Image

```bash
# Raw image
./ALAT/create_image.sh x86 raw

# ISO image (bootable)
./ALAT/create_image.sh x86 iso

# VirtualBox image
./ALAT/create_image.sh x86 vdi

# QEMU image
./ALAT/create_image.sh x86 qcow2

# Ukuran custom
./ALAT/create_image.sh x86 raw -s 128M

# Buat semua tipe
./ALAT/create_image.sh x86 all
```

### Membuat Initramfs

```bash
./ALAT/create_image.sh x86 initrd
```

---

## Troubleshooting

### Error: Compiler tidak ditemukan

```
[ERROR] Compiler arm-linux-gnueabihf-gcc tidak ditemukan
```

**Solusi:**
```bash
# Ubuntu/Debian
sudo apt install gcc-arm-linux-gnueabihf

# Arch Linux
sudo pacman -S arm-linux-gnueabihf-gcc
```

### Error: Multilib tidak tersedia

```
/usr/bin/ld: cannot find -lgcc
/usr/bin/ld: skipping incompatible /usr/lib/gcc/x86_64-linux-gnu/11/libgcc.a
```

**Solusi:**
```bash
sudo apt install gcc-multilib g++-multilib
```

### Error: QEMU tidak bisa menjalankan kernel

```
qemu-system-i386: Unable to load kernel
```

**Solusi:**
1. Pastikan kernel sudah di-build
2. Cek path ke file ELF benar
3. Pastikan format ELF sesuai arsitektur

### Error: Linker script tidak ditemukan

```
[ERROR] Linker script tidak ditemukan: ALAT/linker_x86.ld
```

**Solusi:**
Jalankan dari direktori root proyek:
```bash
cd /path/to/PIGURA
make ARCH=x86
```

### Error: GDB tidak bisa connect

```
localhost:1234: Connection refused
```

**Solusi:**
1. Pastikan QEMU berjalan dengan flag `-s -S`
2. Cek port tidak digunakan aplikasi lain
3. Gunakan `gdb-multiarch` untuk arsitektur non-native

### Build Lambat

**Solusi:**
1. Gunakan parallel build: `make -j4 ARCH=x86`
2. Pastikan disk tidak penuh
3. Gunakan SSD jika memungkinkan

---

## Tips dan Best Practices

### 1. Build Incremental

Make secara otomatis mendeteksi file yang berubah:
```bash
# Hanya rebuild file yang berubah
make ARCH=x86
```

### 2. Clean Build

Jika ada masalah aneh, coba clean build:
```bash
make clean ARCH=x86
make ARCH=x86
```

### 3. Verbose Output

Untuk debugging masalah build:
```bash
make ARCH=x86 VERBOSE=1
```

### 4. Parallel Build

Memanfaatkan multiple cores:
```bash
make -j$(nproc) ARCH=x86
```

### 5. Output ke File

Menyimpan log build:
```bash
make ARCH=x86 2>&1 | tee build.log
```

---

## Ringkasan

Proses build Pigura OS:

1. **Install dependensi** - Compiler, emulator, tools
2. **Konfigurasi** - Edit `Konfigurasi.mk` jika perlu
3. **Build** - `make ARCH=x86` atau `./ALAT/build.sh x86`
4. **Test** - Jalankan di QEMU dengan `./ALAT/qemu_run.sh x86`
5. **Debug** - Gunakan `./ALAT/debug.sh x86` jika ada masalah

Dengan panduan ini, Anda dapat membangun dan menguji Pigura OS di berbagai arsitektur dengan mudah.
