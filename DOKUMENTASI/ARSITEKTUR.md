# ARSITEKTUR SISTEM PIGURA OS

## Daftar Isi

1. [Gambaran Umum](#gambaran-umum)
2. [Model Kernel](#model-kernel)
3. [Hardware Abstraction Layer (HAL)](#hardware-abstraction-layer-hal)
4. [Multi-Arsitektur](#multi-arsitektur)
5. [Pigura C90](#pigura-c90)
6. [Pembagian Bahasa Pemrograman](#pembagian-bahasa-pemrograman)
7. [Boot Process](#boot-process)
8. [Memory Layout](#memory-layout)

---

## Gambaran Umum

Pigura OS adalah sistem operasi **original** yang ditulis dari scratch, bukan fork atau turunan dari Linux maupun Unix. Sistem operasi ini memiliki identitas sendiri dengan filosofi minimalis dan efisiensi maksimal. Nama "Pigura" diambil dari bahasa Indonesia yang berarti "bingkai" atau "frame", mencerminkan konsep dasar sistem ini yang menggunakan framebuffer langsung tanpa lapisan UI yang kompleks seperti X11 atau Wayland.

### Karakteristik Utama

| Aspek | Deskripsi |
|-------|-----------|
| Identitas | ORIGINAL (dari scratch) |
| Kernel | Monolithic |
| Bahasa | Pigura C90 (C89 + POSIX Safe) |
| UI Layer | Library Terintegrasi (~100 KB) |
| Driver | Per-IC Series (~600 KB) |
| Filesystem | FAT32/NTFS/ext2/ISO9660/PiguraFS |
| Arsitektur | x86/x86_64/ARM/ARMv7/ARM64 |

### Perbandingan dengan Sistem Operasi Tradisional

Berikut adalah perbandingan kompleksitas antara sistem operasi tradisional dan Pigura OS:

| Aspek | Linux/Windows | Pigura OS |
|-------|---------------|-----------|
| Identitas | Fork/Turunan | ORIGINAL (dari scratch) |
| UI Layer | X11/Wayland (~500 MB) | Library Terintegrasi (~100 KB) |
| Driver Code | Per-vendor (~50 MB) | Per-IC Series (~600 KB) |
| Runtime | Multiple (~100 MB+) | Minimalist (~50 KB) |
| LibC | glibc (~3000+ fungsi) | Pigura libc (~160 fungsi) |
| Total Estimasi | ~650 MB+ | ~5-6 MB |

---

## Model Kernel

Pigura OS menggunakan model **kernel monolitik** yang dipilih karena beberapa alasan fundamental yang berkaitan dengan performa dan kesederhanaan implementasi. Dalam model ini, seluruh komponen inti seperti scheduler, memory manager, device driver, dan filesystem berjalan dalam satu ruang alamat kernel. Pendekatan ini mengeliminasi overhead komunikasi inter-process yang ada pada microkernel, sehingga menghasilkan performa yang lebih baik untuk sistem operasi yang ditargetkan untuk penggunaan personal.

### Komponen Kernel

```
+----------------------------------------------------------+
|                     KERNEL SPACE                          |
+----------------------------------------------------------+
|  +-------------+  +-------------+  +-------------+       |
|  |  Scheduler  |  |   Memory    |  |    VFS      |       |
|  |   (proses)  |  |  Manager    |  | (berkas)    |       |
|  +-------------+  +-------------+  +-------------+       |
|  +-------------+  +-------------+  +-------------+       |
|  |     HAL     |  |   Device    |  |   System    |       |
|  |             |  |  Drivers    |  |   Calls     |       |
|  +-------------+  +-------------+  +-------------+       |
+----------------------------------------------------------+
                            |
                            v
+----------------------------------------------------------+
|                     USER SPACE                            |
+----------------------------------------------------------+
|  Shell  |  Aplikasi  |  LibPigura  |  Dekor             |
+----------------------------------------------------------+
```

### Keuntungan Kernel Monolitik

1. **Performa Tinggi**: Tidak ada overhead context switch untuk system call
2. **Implementasi Sederhana**: Satu ruang alamat, debug lebih mudah
3. **Komunikasi Efisien**: Komponen kernel berbagi memori langsung
4. **Kode Kompak**: Tidak perlu stub dan proxy untuk IPC

---

## Hardware Abstraction Layer (HAL)

HAL adalah lapisan yang memisahkan kode kernel dari detail spesifik hardware. Dengan HAL, kode kernel dapat berjalan di berbagai arsitektur tanpa modifikasi. HAL menyediakan interface yang konsisten untuk operasi-operasi hardware fundamental.

### Interface HAL

| Interface | Fungsi | Deskripsi |
|-----------|--------|-----------|
| `hal_cpu` | Operasi CPU | halt, restart, get_id, cache ops |
| `hal_memori` | Manajemen Memori | paging, allocation, mapping |
| `hal_interupsi` | Handler Interupsi | register, enable, disable |
| `hal_timer` | Timer | delay, get_ticks, set_frequency |
| `hal_console` | Output Console | putchar, puts, clear |

### Struktur HAL

```
SUMBER/inti/hal/
├── hal.h              # Header interface HAL
├── hal.c              # Implementasi umum
├── hal_cpu.c          # Operasi CPU
├── hal_memori.c       # Manajemen memori
├── hal_interupsi.c    # Handler interupsi
├── hal_timer.c        # Timer interface
└── hal_console.c      # Console output
```

### Implementasi Per-Arsitektur

Setiap arsitektur mengimplementasikan HAL-nya sendiri:

| Arsitektur | File Implementasi | Keterangan |
|------------|-------------------|------------|
| x86 | `arsitektur/x86/hal_x86.c` | Intel/AMD 32-bit |
| x86_64 | `arsitektur/x86_64/hal_x86_64.c` | Intel/AMD 64-bit |
| ARM | `arsitektur/arm/hal_arm.c` | ARM 32-bit generik |
| ARMv7 | `arsitektur/armv7/hal_armv7.c` | ARM Cortex-A series |
| ARM64 | `arsitektur/arm64/hal_arm64.c` | AArch64 64-bit |

---

## Multi-Arsitektur

Pigura OS dirancang dari awal untuk mendukung multiple arsitektur dengan satu basis kode sumber. Sistem build menggunakan parameter `ARCH` untuk memilih target kompilasi.

### Arsitektur yang Didukung

| Arsitektur | Bit | Compiler | Target Platform |
|------------|-----|----------|-----------------|
| x86 | 32-bit | i686-pigura-gcc | Desktop legacy |
| x86_64 | 64-bit | x86_64-pigura-gcc | Desktop modern |
| ARM | 32-bit | arm-pigura-gcc | Embedded legacy |
| ARMv7 | 32-bit | armv7-pigura-gcc | Mobile/Embedded |
| ARM64 | 64-bit | aarch64-pigura-gcc | Mobile modern |

### Struktur Direktori Arsitektur

```
SUMBER/arsitektur/
├── x86/                  # Intel/AMD 32-bit
│   ├── boot/             # Boot sector, protected mode
│   │   ├── bootsect.S    # Boot sector
│   │   ├── protected_mode.S
│   │   ├── multiboot.S
│   │   ├── paging_setup.S
│   │   └── stage2.c
│   ├── memori/           # Paging, GDT, segment
│   │   ├── gdt_x86.c
│   │   ├── paging_x86.c
│   │   ├── segment_x86.c
│   │   └── ldt_x86.c
│   ├── proses/           # Context, TSS, user mode
│   │   ├── context_x86.S
│   │   ├── tss_x86.c
│   │   ├── user_mode_x86.c
│   │   ├── elf_x86.c
│   │   └── ring_switch_x86.S
│   ├── interupsi/        # IDT, ISR, IRQ
│   │   ├── idt_x86.c
│   │   ├── isr_x86.S
│   │   ├── irq_x86.c
│   │   └── exception_x86.c
│   ├── hal_x86.c
│   ├── cpu_x86.c
│   ├── memori_x86.c
│   ├── interupsi_x86.c
│   ├── proses_x86.c
│   └── syscall_x86.S
│
├── x86_64/               # Intel/AMD 64-bit
│   ├── boot/             # Long mode setup
│   │   ├── bootsect.S
│   │   ├── long_mode.S
│   │   ├── paging_setup.S
│   │   └── stage2.c
│   ├── memori/           # PML4, paging 64-bit
│   │   ├── gdt_x86_64.c
│   │   ├── paging_x86_64.c
│   │   └── pml4_x86_64.c
│   ├── proses/           # Context 64-bit
│   ├── interupsi/        # IDT 64-bit
│   └── ... (mirip x86)
│
├── arm/                  # ARM 32-bit generik
│   ├── boot/             # Vector table, MMU setup
│   │   ├── boot_arm.S
│   │   ├── mmu_setup.S
│   │   └── vector_table.S
│   ├── memori/           # Page tables, TLB
│   │   ├── mmu_arm.c
│   │   ├── page_table_arm.c
│   │   └── tlb_arm.c
│   ├── proses/           # Context switching
│   ├── interupsi/        # GIC, exceptions
│   └── ...
│
├── armv7/                # ARM Cortex-A series
│   ├── boot/             # VFP, NEON setup
│   │   ├── boot_armv7.S
│   │   ├── mmu_setup.S
│   │   ├── vfp_setup.S
│   │   └── neon_setup.S
│   ├── memori/           # L2 cache
│   │   ├── mmu_armv7.c
│   │   ├── page_table_armv7.c
│   │   └── l2cache_armv7.c
│   └── ...
│
└── arm64/                # AArch64 64-bit
    ├── boot/             # EL levels setup
    │   ├── boot_arm64.S
    │   ├── mmu_setup.S
    │   └── vector_table.S
    ├── memori/           # 4-level paging
    │   ├── mmu_arm64.c
    │   ├── page_table_arm64.c
    │   └── tlb_arm64.c
    └── ...
```

---

## Pigura C90

Pigura C90 adalah definisi bahasa khusus untuk Pigura OS yang menggabungkan standar C89 dengan fungsi-fungsi aman dari POSIX.

### Definisi

```
PIGURA C90 = C89 Core + POSIX Safe Functions
```

### Komponen

| Komponen | Deskripsi |
|----------|-----------|
| C89 Core | Standar ANSI C yang mature dan portabel |
| POSIX Safe Functions | Fungsi dengan bounds checking untuk keamanan |

### Keunggulan

- Tidak perlu macro `_POSIX_C_SOURCE` atau `_BSD_SOURCE`
- Satu standard terpadu, tidak ada dualisme
- Semua fungsi aman tersedia secara default
- Portabilitas maksimal ke berbagai arsitektur

### Filosofi "Satu Fungsi, Satu Tujuan"

Dalam Pigura C90, setiap tugas hanya memiliki SATU fungsi - versi aman:

| Dihapus (Unsafe) | Tersedia (Safe) |
|------------------|-----------------|
| `sprintf` | `snprintf` (bounded) |
| `strcpy` | `strncpy` (bounded) |
| `strcat` | `strncat` (bounded) |
| `gets` | `fgets` (bounded) |
| `strtok` | `strtok_r` (reentrant) |

---

## Pembagian Bahasa Pemrograman

Pembagian bahasa pemrograman dalam Pigura OS mengikuti prinsip "hanya assembly untuk yang benar-benar diperlukan".

### Pigura C90

Digunakan untuk seluruh kernel core, driver, filesystem, libc, libpigura, dan aplikasi karena:
- Portabilitas tinggi
- Keamanan (fungsi bounded)
- Kemudahan maintenance
- Standar matang dan didukung luas

### Inline Assembly

Hanya digunakan untuk operasi-operasi vital yang memerlukan akses hardware langsung:

| Operasi | Alasan |
|---------|--------|
| I/O port (inb, outb, inw, outw, inl, outl) | Akses hardware langsung |
| Context switch | Operasi kritis yang membutuhkan kontrol penuh |
| Interrupt handler entry | Instruksi khusus CPU |
| Ring switch (0 ke 3) | Kontrol register segment |
| CPUID, MSR access | Instruksi CPU spesifik |
| TLB flush, cache ops | Instruksi privileged |

---

## Boot Process

### Alur Boot x86/x86_64

```
+------------------+
|    BIOS/UEFI     |  Power-on, POST
+--------+---------+
         |
         v
+------------------+
|   Boot Sector    |  512 byte, MBR
|   (bootsect.S)   |  Load stage 2
+--------+---------+
         |
         v
+------------------+
|    Stage 2       |  Load kernel
|   (stage2.c)     |  Setup environment
+--------+---------+
         |
         v
+------------------+
| Protected Mode   |  Enable A20, GDT
| (protected_mode.S)| 32-bit mode
+--------+---------+
         |
         v
+------------------+
|   Paging Setup   |  Enable paging
| (paging_setup.S) | Higher half kernel
+--------+---------+
         |
         v
+------------------+
|  kernel_main()   |  Entry point C
+------------------+
```

### Alur Boot ARM

```
+------------------+
|  Vector Table    |  Exception vectors
| (vector_table.S) |  Reset handler
+--------+---------+
         |
         v
+------------------+
|  Boot Assembly   |  Setup stack
|  (boot_arm.S)    |  Clear BSS
+--------+---------+
         |
         v
+------------------+
|   MMU Setup      |  Enable MMU
|  (mmu_setup.S)   |  Configure page tables
+--------+---------+
         |
         v
+------------------+
|  kernel_main()   |  Entry point C
+------------------+
```

---

## Memory Layout

### x86 (32-bit) Memory Map

```
+------------------+ 0xFFFFFFFF
|    Kernel Space  |  1 GB
|    (Higher Half) |
+------------------+ 0xC0000000
|                  |
|    User Space    |  3 GB
|                  |
+------------------+ 0x00100000 (1 MB)
|    Kernel        |
|    (Physical)    |
+------------------+ 0x00000000
```

### x86_64 (64-bit) Memory Map

```
+------------------+ 0xFFFFFFFFFFFFFFFF
|    Kernel Space  |  Higher Half
|    (-2GB)        |
+------------------+ 0xFFFFFFFF80000000
|                  |
|    User Space    |  128 TB
|                  |
+------------------+ 0x00007FFFFFFFFFFF
|    Non-Canonical |  Hole
+------------------+ 0x0000800000000000
|    Kernel Space  |
+------------------+ 0x0000000000000000
```

### ARM Memory Map

```
+------------------+ 0xFFFFFFFF
|    Kernel Space  |  Higher vectors
+------------------+ 0xFFFF0000
|                  |
|                  |
+------------------+ 0xC0000000 (3 GB)
|    Kernel        |
|    (Higher Half) |
+------------------+ 0x40000000 (1 GB)
|    Physical RAM  |
+------------------+ 0x00000000
```

---

## Ringkasan

Arsitektur Pigura OS dirancang dengan prinsip:

1. **Original** - Bukan fork, ditulis dari scratch
2. **Minimalis** - Hanya komponen yang diperlukan
3. **Portabel** - Multi-arsitektur dengan HAL
4. **Aman** - Fungsi bounded, compositor isolasi
5. **Efisien** - Kernel monolitik, tanpa overhead IPC

Dengan pendekatan ini, Pigura OS menjadi sistem operasi yang mudah dipahami, dipelihara, dan memiliki identitas sendiri.