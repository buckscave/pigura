# STRUKTUR DIREKTORI PIGURA OS

## Daftar Isi

1. [Gambaran Umum](#gambaran-umum)
2. [Struktur Root](#struktur-root)
3. [Direktori SUMBER](#direktori-sumber)
4. [Direktori Inti Kernel](#direktori-inti-kernel)
5. [Direktori Arsitektur](#direktori-arsitektur)
6. [Direktori LibC](#direktori-libc)
7. [Direktori Perangkat](#direktori-perangkat)
8. [Direktori Pigura](#direktori-pigura)
9. [Direktori Format](#direktori-format)
10. [Direktori Berkas](#direktori-berkas)
11. [Direktori Aplikasi](#direktori-aplikasi)
12. [Direktori ALAT](#direktori-alat)
13. [Direktori DOKUMENTASI](#direktori-dokumentasi)

---

## Gambaran Umum

Pigura OS menggunakan struktur direktori yang terorganisir dengan pembagian tanggung jawab yang jelas. Setiap direktori memiliki fungsi spesifik yang mendukung keseluruhan sistem operasi. Struktur ini dirancang untuk memudahkan navigasi, pengembangan, dan maintenance kode sumber.

### Filosofi Struktur

Pigura OS mengadopsi pendekatan **All-in-One Graphics Layer** dimana seluruh komponen grafis (Compositor, Window Manager, Toolkit, Desktop Environment) terintegrasi dalam satu modul bernama `pigura/`. Hal ini berbeda dengan arsitektur Linux yang memisahkan X11/Wayland, GTK/QT, dan desktop environment sebagai komponen terpisah.

### Estimasi Ukuran Komponen

| Komponen | Jumlah File | Estimasi Ukuran |
|----------|-------------|-----------------|
| Kernel Core | ~30 | ~100 KB |
| LibC | 73 | ~150 KB |
| Pigura (All-in-One) | ~80 | ~200 KB |
| IC Detection System | ~25 | ~60 KB |
| Filesystem Drivers | ~45 | ~120 KB |
| Architecture (per arch) | ~20 | ~50 KB |
| Device Drivers | ~20 | ~60 KB |
| Bootloader | ~5 | ~10 KB |
| Aplikasi Dasar | ~35 | ~100 KB |
| **TOTAL** | **~350 file** | **~6-7 MB** |

---

## Struktur Root

```
PIGURA/
├── SUMBER/                 # Kode sumber utama
├── ALAT/                   # Peralatan build dan skrip
├── DOKUMENTASI/            # Dokumentasi proyek
├── CITRA/                  # Gambar dan asset
├── BUILD/                  # Output build (generated)
├── OUTPUT/                 # Output final (generated)
├── IMAGE/                  # Disk images (generated)
├── Makefile                # File build utama
├── Konfigurasi.mk          # Konfigurasi build
├── pigura.txt              # Dokumen desain utama
├── BACALAH.md              # README proyek
└── LISENSI                 # File lisensi
```

---

## Direktori SUMBER

Direktori `SUMBER/` berisi seluruh kode sumber Pigura OS yang diorganisir berdasarkan fungsi dan tanggung jawab.

```
SUMBER/
├── inti/                   # Kernel core
├── arsitektur/             # Kode arsitektur-spesifik
├── libc/                   # C library terpadu
├── pigura/                 # All-in-One Graphics Layer
├── perangkat/              # Driver perangkat
├── berkas/                 # Driver filesystem
├── format/                 # Handler format file
├── aplikasi/               # Aplikasi user
└── _memuat/                # Bootloader
```

---

## Direktori Inti Kernel

Direktori `SUMBER/inti/` berisi komponen inti kernel yang bersifat arsitektur-independen.

```
SUMBER/inti/
├── kernel.h                # Header utama kernel
├── types.h                 # Definisi tipe data
├── config.h                # Konfigurasi kernel
├── konstanta.h             # Konstanta sistem
├── makro.h                 # Makro utilitas
├── panic.h                 # Panic handling
├── stdarg.h                # Variadic arguments
│
├── boot/                   # Inisialisasi boot
│   ├── kernel_main.c       # Entry point kernel
│   ├── inisialisasi.c      # Inisialisasi subsistem
│   ├── multiboot.c         # Parser multiboot
│   ├── panic.c             # Kernel panic handler
│   ├── string_mem.c        # String dan mem utilitas
│   └── setup_stack.c       # Setup stack awal
│
├── hal/                    # Hardware Abstraction Layer
│   ├── hal.h               # Header HAL
│   ├── hal.c               # Core HAL
│   ├── hal_cpu.c           # HAL CPU
│   ├── hal_memori.c        # HAL memori
│   ├── hal_interupsi.c     # HAL interupsi
│   ├── hal_console.c       # HAL console
│   └── hal_timer.c         # HAL timer
│
├── memori/                 # Memory management
│   ├── memori.c            # Core memory manager
│   ├── fisik.c             # Physical memory
│   ├── virtual.c           # Virtual memory
│   ├── paging.c            # Paging system
│   ├── heap.c              # Kernel heap
│   ├── allocator.c         # Memory allocator
│   ├── dma_buffer.c        # DMA buffers
│   └── kmap.c              # Kernel mapping
│
├── proses/                 # Process management
│   ├── proses.c            # Core process
│   ├── scheduler.c         # Scheduler
│   ├── context.c           # Context management
│   ├── fork.c              # Process fork
│   ├── exec.c              # Program execution
│   ├── exit.c              # Process exit
│   ├── wait.c              # Wait for child
│   ├── signal.c            # Signal handling
│   ├── thread.c            # Threading
│   ├── tss.c               # Task state segment
│   ├── user_mode.c         # User mode transition
│   ├── elf.c               # ELF loader
│   └── ring_switch.c       # Ring switching
│
├── interupsi/              # Interrupt handling
│   ├── interupsi.c         # Core interrupt
│   ├── exception.c         # Exception handlers
│   ├── pic.c               # PIC controller
│   ├── apic.c              # APIC controller
│   ├── irq.c               # IRQ handling
│   ├── isr.c               # ISR management
│   └── handler.c           # Handler registration
│
└── syscall/                # System calls
    ├── syscall.c           # Core syscall
    ├── tabel.c             # Syscall table
    ├── dispatcher.c        # Call dispatcher
    ├── validasi.c          # Parameter validation
    └── syscall_user.c      # User-space interface
```

---

## Direktori Arsitektur

Direktori `SUMBER/arsitektur/` berisi kode yang spesifik untuk setiap arsitektur yang didukung.

```
SUMBER/arsitektur/
│
├── x86/                    # Intel/AMD 32-bit
│   ├── hal_x86.c           # HAL implementation
│   ├── cpu_x86.c           # CPU operations
│   ├── memori_x86.c        # Memory ops
│   ├── interupsi_x86.c     # Interrupt ops
│   ├── proses_x86.c        # Process ops
│   │
│   ├── boot/               # Boot code
│   │   ├── bootsect.S      # Boot sector
│   │   ├── protected_mode.S# Protected mode
│   │   ├── multiboot.S     # Multiboot header
│   │   ├── paging_setup.S  # Paging setup
│   │   └── stage2.c        # Stage 2 loader
│   │
│   ├── memori/             # Memory management
│   │   ├── gdt_x86.c       # Global Descriptor Table
│   │   ├── ldt_x86.c       # Local Descriptor Table
│   │   ├── paging_x86.c    # Paging implementation
│   │   └── segment_x86.c   # Segmentation
│   │
│   ├── proses/             # Process management
│   │   ├── context_x86.S   # Context switching
│   │   ├── tss_x86.c       # Task State Segment
│   │   ├── user_mode_x86.c # User mode switch
│   │   ├── elf_x86.c       # ELF loader
│   │   └── ring_switch_x86.S
│   │
│   ├── interupsi/          # Interrupt handling
│   │   ├── idt_x86.c       # Interrupt Descriptor Table
│   │   ├── isr_x86.S       # ISR stubs
│   │   ├── irq_x86.c       # IRQ handling
│   │   └── exception_x86.c # Exception handlers
│   │
│   └── syscall_x86.S       # Syscall entry
│
├── x86_64/                 # Intel/AMD 64-bit
│   ├── hal_x86_64.c
│   ├── cpu_x86_64.c
│   ├── memori_x86_64.c
│   ├── interupsi_x86_64.c
│   ├── proses_x86_64.c
│   │
│   ├── boot/
│   │   ├── bootsect.S
│   │   ├── long_mode.S     # Long mode setup
│   │   ├── paging_setup.S
│   │   └── stage2.c
│   │
│   ├── memori/
│   │   ├── gdt_x86_64.c
│   │   ├── paging_x86_64.c
│   │   └── pml4_x86_64.c   # 4-level paging
│   │
│   ├── proses/
│   │   ├── context_x86_64.S
│   │   ├── tss_x86_64.c
│   │   ├── user_mode_x86_64.c
│   │   ├── elf_x86_64.c
│   │   └── ring_switch_x86_64.S
│   │
│   ├── interupsi/
│   │   ├── idt_x86_64.c
│   │   ├── isr_x86_64.S
│   │   ├── irq_x86_64.c
│   │   └── exception_x86_64.c
│   │
│   └── syscall_x86_64.S
│
├── arm/                    # ARM 32-bit legacy
│   ├── hal_arm.c
│   ├── cpu_arm.c
│   ├── memori_arm.c
│   ├── interupsi_arm.c
│   ├── proses_arm.c
│   │
│   ├── boot/
│   │   ├── boot_arm.S
│   │   ├── mmu_setup.S
│   │   └── vector_table.S
│   │
│   ├── memori/
│   │   ├── mmu_arm.c
│   │   ├── page_table_arm.c
│   │   └── tlb_arm.c
│   │
│   ├── proses/
│   │   ├── context_arm.S
│   │   ├── user_mode_arm.c
│   │   └── elf_arm.c
│   │
│   ├── interupsi/
│   │   ├── gic_arm.c
│   │   ├── interrupt_arm.c
│   │   └── exception_arm.c
│   │
│   └── syscall_arm.S
│
├── armv7/                  # ARM Cortex-A series
│   ├── hal_armv7.c
│   ├── cpu_armv7.c
│   ├── memori_armv7.c
│   ├── interupsi_armv7.c
│   ├── proses_armv7.c
│   │
│   ├── boot/
│   │   ├── boot_armv7.S
│   │   ├── mmu_setup.S
│   │   ├── vfp_setup.S     # VFP coprocessor
│   │   ├── neon_setup.S    # NEON SIMD
│   │   └── vector_table.S
│   │
│   ├── memori/
│   │   ├── mmu_armv7.c
│   │   ├── page_table_armv7.c
│   │   └── l2cache_armv7.c # L2 cache
│   │
│   ├── proses/
│   │   ├── context_armv7.S
│   │   ├── user_mode_armv7.c
│   │   └── elf_armv7.c
│   │
│   ├── interupsi/
│   │   ├── gic_armv7.c     # GICv2/v3
│   │   ├── interrupt_armv7.c
│   │   └── exception_armv7.c
│   │
│   └── syscall_armv7.S
│
└── arm64/                  # AArch64 64-bit
    ├── hal_arm64.c
    ├── cpu_arm64.c
    ├── memori_arm64.c
    ├── interupsi_arm64.c
    ├── proses_arm64.c
    │
    ├── boot/
    │   ├── boot_arm64.S
    │   ├── mmu_setup.S
    │   └── vector_table.S
    │
    ├── memori/
    │   ├── mmu_arm64.c
    │   ├── page_table_arm64.c
    │   └── tlb_arm64.c
    │
    ├── proses/
    │   ├── context_arm64.S
    │   ├── user_mode_arm64.c
    │   └── elf_arm64.c
    │
    ├── interupsi/
    │   ├── gicv3_arm64.c    # GICv3
    │   ├── interrupt_arm64.c
    │   └── exception_arm64.c
    │
    └── syscall_arm64.S
```

---

## Direktori LibC

Direktori `SUMBER/libc/` berisi library C terpadu dengan 73 file dan ~160 fungsi aman.

```
SUMBER/libc/
├── include/                # Header publik (16 file)
│   ├── stdio.h
│   ├── stdlib.h
│   ├── string.h
│   ├── stdint.h
│   ├── stddef.h
│   ├── stdarg.h
│   ├── ctype.h
│   ├── errno.h
│   ├── limits.h
│   ├── assert.h
│   ├── unistd.h
│   ├── fcntl.h
│   ├── signal.h
│   ├── time.h
│   ├── math.h
│   ├── setjmp.h
│   └── sys/
│       ├── types.h
│       ├── stat.h
│       ├── wait.h
│       ├── mman.h
│       ├── socket.h
│       ├── ioctl.h
│       └── uio.h
│
├── internal/               # Internal implementation
│   ├── syscall.S           # Syscall entry
│   ├── errno.c             # Global errno
│   ├── startup.c           # _start entry
│   └── arch/               # Arch-specific
│       ├── x86/syscall.S
│       ├── x86_64/syscall.S
│       ├── arm/syscall.S
│       ├── armv7/syscall.S
│       └── arm64/syscall.S
│
├── stdio/                  # I/O stream (11 file)
│   ├── printf.c
│   ├── snprintf.c
│   ├── puts.c
│   ├── gets.c
│   ├── file.c
│   ├── seek.c
│   ├── flush.c
│   ├── error.c
│   ├── remove.c
│   ├── tmp.c
│   └── stdio.h
│
├── stdlib/                 # Utilitas (9 file)
│   ├── mem.c
│   ├── conv.c
│   ├── rand.c
│   ├── exit.c
│   ├── system.c
│   ├── getenv.c
│   ├── bsearch.c
│   ├── qsort.c
│   └── abs.c
│
├── string/                 # String ops (6 file)
│   ├── mem.c
│   ├── str.c
│   ├── strcopy.c
│   ├── strfind.c
│   ├── strutil.c
│   └── strerror.c
│
├── ctype/                  # Character (1 file)
│   └── ctype.c
│
├── unistd/                 # POSIX (6 file)
│   ├── io.c
│   ├── file.c
│   ├── proc.c
│   ├── cwd.c
│   ├── sleep.c
│   └── unlink.c
│
├── fcntl/                  # File control (1 file)
│   └── fcntl.c
│
├── signal/                 # Signal (1 file)
│   └── signal.c
│
├── time/                   # Time (3 file)
│   ├── time.c
│   ├── ctime.c
│   └── strftime.c
│
├── math/                   # Math (1 file)
│   └── math.c
│
├── setjmp/                 # Non-local jump (1 file)
│   └── setjmp.c
│
└── network/                # Network (3 file)
    ├── socket.c
    ├── send.c
    └── netdb.c
```

---

## Direktori Perangkat

Direktori `SUMBER/perangkat/` berisi driver perangkat dan sistem IC Detection.

```
SUMBER/perangkat/
│
├── ic/                     # IC Detection System
│   ├── ic_deteksi.c        # Engine deteksi
│   ├── ic_mesin.c          # Core engine
│   ├── ic_klasifikasi.c    # Klasifikasi IC
│   ├── ic_parameter.c      # Parameter handling
│   ├── ic_validasi.c       # Validasi parameter
│   │
│   ├── database/           # Database parameter
│   │   ├── database.c
│   │   ├── db_cpu.c
│   │   ├── db_gpu.c
│   │   ├── db_network.c
│   │   ├── db_storage.c
│   │   ├── db_display.c
│   │   ├── db_audio.c
│   │   ├── db_input.c
│   │   └── db_usb.c
│   │
│   ├── probe/              # Probe engine
│   │   ├── probe.c
│   │   ├── probe_pci.c
│   │   ├── probe_usb.c
│   │   ├── probe_i2c.c
│   │   ├── probe_spi.c
│   │   └── probe_mmio.c
│   │
│   └── driver_umum/        # Driver generik
│       ├── driver_umum.c
│       ├── penyimpanan_nvme_umum.c
│       ├── penyimpanan_ata_umum.c
│       ├── audio_hda_umum.c
│       ├── display_vga_umum.c
│       ├── display_hdmi_umum.c
│       ├── net_ethernet_umum.c
│       ├── net_wifi_umum.c
│       └── input_usb_hid_umum.c
│
├── tampilan/               # Display devices
│   │
│   ├── video/              # Driver hardware video/display
│   │   ├── video.c         # Modul utama video
│   │   ├── video_init.c    # Inisialisasi video
│   │   ├── vbe.c           # VESA BIOS Extensions (BIOS)
│   │   └── uefi_gop.c      # UEFI Graphics Output Protocol
│   │
│   └── gpu/                # Driver GPU
│       ├── gpu.c           # Core GPU driver
│       ├── gpu_deteksi.c   # Deteksi via IC Detection
│       ├── gpu_init.c      # Inisialisasi GPU
│       ├── gpu_render.c    # Render commands
│       ├── gpu_2d.c        # 2D acceleration
│       ├── gpu_3d.c        # 3D acceleration
│       ├── gpu_memori.c    # GPU memory management
│       ├── gpu_command.c   # Command buffer
│       └── gpu_shader.c    # Shader handling
│
├── masukan/                # Input devices
│   ├── masukan.c
│   ├── papanketik.c        # Keyboard
│   ├── tetikus.c           # Mouse
│   ├── layarsentuh.c       # Touchscreen
│   ├── gamepad.c
│   ├── joystick.c
│   └── hid.c               # USB HID
│
├── penyimpanan/            # Storage
│   ├── penyimpanan.c
│   ├── ata.c               # IDE/PATA
│   ├── ahci.c              # SATA
│   ├── nvme.c              # NVMe
│   ├── sd_card.c
│   ├── mmc.c
│   ├── partisi.c
│   ├── mbr.c
│   ├── gpt.c
│   └── penyimpanan_usb.c
│
├── jaringan/               # Network
│   ├── jaringan.c
│   ├── netdev.c
│   ├── ethernet.c
│   ├── wifi.c
│   ├── tcp.c
│   ├── udp.c
│   ├── socket.c
│   ├── dns.c
│   ├── dhcp.c
│   └── arp.c
│
├── cpu/                    # CPU features
│   ├── cpu.c
│   ├── cpuid.c
│   ├── apic.c
│   ├── smp.c
│   ├── acpi.c
│   ├── cache.c
│   ├── topologi.c
│   ├── fpu.c
│   ├── sse.c
│   └── avx.c
│
├── memori/                 # Memory devices
│   ├── pengatur_memori.c
│   ├── ram.c
│   ├── pengatur_cache.c
│   └── pci.c
│
└── dma/                    # DMA
    ├── dma.c
    ├── dma_transfer.c
    └── dma_controller.c
```

### Penjelasan Struktur Tampilan

Struktur `perangkat/tampilan/` diorganisir sebagai berikut:

| Direktori | Fungsi | Isi |
|-----------|--------|-----|
| `video/` | Driver hardware display | VBE, UEFI GOP, Video init |
| `gpu/` | Driver akselerasi grafis | 2D, 3D, Shader, Memory |

**Penting:** Tidak ada `console/` atau `framebuffer/` di level driver karena semua rendering text maupun grafis ditangani oleh layer `pigura/` (all-in-one graphics layer).

---

## Direktori Pigura

Direktori `SUMBER/pigura/` adalah **All-in-One Graphics Layer** yang mengintegrasikan Compositor, Window Manager, Toolkit, dan Desktop Environment dalam satu modul terpadu. Ini adalah inovasi utama Pigura OS yang membedakannya dari arsitektur Linux.

### Filosofi

```
┌─────────────────────────────────────────────────────────────────┐
│                      LINUX TRADISIONAL                          │
├─────────────────────────────────────────────────────────────────┤
│  Aplikasi → GTK/QT → X11/Wayland → KMS/DRM → Framebuffer       │
│              ↑________↑   ↑________↑    ↑______↑                │
│               Toolkit     Display Srv    Driver                 │
│  Total: ~500 MB kode, 4 layer terpisah                         │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                      PIGURA OS                                  │
├─────────────────────────────────────────────────────────────────┤
│  Aplikasi → PIGURA (All-in-One) → Video Driver → Framebuffer   │
│              ↑___________________↑                               │
│         Compositor + WM + Toolkit + DE                          │
│  Total: ~200 KB kode, 1 layer terintegrasi                      │
└─────────────────────────────────────────────────────────────────┘
```

### Struktur Direktori

```
SUMBER/pigura/
│
├── pigura.c                # Entry point utama
│
├── framebuffer/            # Software Rendering & Buffer Management
│   ├── framebuffer.c       # Core framebuffer handling
│   ├── framebuffer_blit.c  # Blitting operations
│   ├── framebuffer_render.c# Render ke framebuffer
│   │
│   ├── akselerasi_gpu/     # GPU Acceleration
│   │   ├── gpu_accel.c     # Core acceleration
│   │   ├── gpu_blit.c      # GPU blit
│   │   ├── gpu_fill.c      # GPU fill
│   │   ├── gpu_copy.c      # GPU copy
│   │   ├── gpu_scale.c     # GPU scale
│   │   ├── gpu_rotate.c    # GPU rotate
│   │   └── gpu_blend.c     # GPU alpha blend
│   │
│   ├── buffer/             # Buffer Management
│   │   ├── buffer.c        # Core buffer
│   │   ├── bufferbelakang.c# Back buffer
│   │   ├── permukaan.c     # Surface management
│   │   ├── dampak.c        # Damage tracking
│   │   └── tukar.c         # Buffer swap
│   │
│   ├── kanvas/             # Canvas/Drawing Surface
│   │   ├── kanvas.c        # Core kanvas
│   │   ├── kanvas_buat.c   # Create kanvas
│   │   ├── kanvas_hapus.c  # Destroy kanvas
│   │   ├── kanvas_ubahsuai.c# Resize kanvas
│   │   ├── kanvas_flip.c   # Flip buffer
│   │   └── kanvas_blit.c   # Blit antar kanvas
│   │
│   └── pengolah/           # Rendering Engine
│       ├── pengolah.c      # Core processor
│       ├── pengolah_cpu.c  # CPU software rendering
│       ├── pengolah_gpu.c  # GPU hardware rendering
│       ├── pengolah_hybrid.c# CPU+GPU hybrid
│       ├── primitif.c      # Primitive dispatcher
│       ├── titik.c         # Draw point
│       ├── garis.c         # Draw line
│       ├── kotak.c         # Draw rectangle
│       ├── lingkaran.c     # Draw circle
│       ├── elip.c          # Draw ellipse
│       ├── poligon.c       # Draw polygon
│       ├── busur.c         # Draw arc
│       ├── kurva.c         # Draw bezier curve
│       └── isi.c           # Fill shapes
│
├── penata/                 # COMPOSITOR
│   │
│   ├── inti/               # Core compositor
│   │   ├── penata.c        # Main interface
│   │   ├── init.c          # Inisialisasi
│   │   ├── hancurkan.c     # Cleanup
│   │   └── pengolah.c      # Render loop
│   │
│   ├── buffer/             # Compositor buffers
│   │   ├── buffer.c
│   │   ├── bufferbelakang.c
│   │   ├── permukaan.c
│   │   ├── dampak.c
│   │   └── tukar.c
│   │
│   ├── klip/               # Clipping
│   │   ├── klip.c
│   │   ├── region.c
│   │   ├── kotak.c
│   │   ├── union.c
│   │   └── intersect.c
│   │
│   ├── lapisan/            # Z-order management
│   │   ├── lapisan.c
│   │   ├── z_order.c
│   │   ├── tumpuk.c
│   │   ├── naik.c
│   │   └── rendah.c
│   │
│   └── efek/               # Visual effects
│       ├── efek.c
│       ├── shadow.c
│       ├── blur.c
│       ├── transparan.c
│       └── animasi.c
│
├── jendela/                # WINDOW MANAGER
│   ├── jendela.c           # Window management
│   ├── wm.c                # Window manager core
│   ├── dekorasi.c          # Window decorations
│   ├── peristiwa.c         # Window events
│   └── z_order.c           # Window stacking
│
├── peristiwa/              # EVENT HANDLING
│   ├── peristiwa.c         # Event system
│   ├── pengendali.c        # Event dispatcher
│   ├── masukan.c           # Input events
│   ├── penunjuk.c          # Pointer events
│   └── fokus.c             # Focus management
│
├── teks/                   # TEXT & FONT RENDERING
│   ├── teks.c              # Text rendering
│   ├── font.c              # Font management
│   ├── font_bitmap.c       # Bitmap font
│   ├── font_ttf.c          # TTF font (opsional)
│   ├── font_cache.c        # Font cache
│   ├── glyph.c             # Glyph handling
│   ├── ukuran.c            # Text measurement
│   └── pengolah_teks.c     # Text processor
│
└── widget/                 # TOOLKIT / WIDGETS
    ├── widget.c            # Widget framework
    ├── tombol.c            # Button widget
    ├── kotakteks.c         # Textbox widget
    ├── kotakcentang.c      # Checkbox widget
    ├── bargulir.c          # Scrollbar widget
    ├── menu.c              # Menu widget
    └── dialog.c            # Dialog widget
```

### Komponen Terintegrasi

| Komponen | Lokasi | Fungsi |
|----------|--------|--------|
| **Compositor** | `penata/` | Menggabungkan output aplikasi ke framebuffer |
| **Window Manager** | `jendela/` | Mengelola posisi dan state jendela |
| **Event Handler** | `peristiwa/` | Routing input ke aplikasi |
| **Toolkit** | `widget/` | Komponen UI siap pakai |
| **Text Engine** | `teks/` | Rendering teks dan font |
| **Framebuffer** | `framebuffer/` | Software rendering dan buffer |

---

## Direktori Format

Direktori `SUMBER/format/` berisi handler format file universal.

```
SUMBER/format/
│
├── format.c                # Core format handler
├── format_registry.c       # Format registration
│
├── gambar/                 # Image formats
│   ├── gambar.c
│   ├── png.c
│   ├── png_baca.c
│   ├── png_tulis.c
│   ├── jpeg.c
│   ├── gif.c
│   ├── bmp.c
│   ├── tga.c
│   ├── svg.c
│   └── ico.c
│
├── dokumen/                # Document formats
│   ├── dokumen.c
│   ├── pdf.c
│   ├── pdf_baca.c
│   ├── pdf_tulis.c
│   ├── txt.c
│   ├── rtf.c
│   ├── html.c
│   ├── xml.c
│   ├── json.c
│   └── markdown.c
│
├── media/                  # Media formats
│   ├── media.c
│   ├── wav.c
│   ├── mp3.c
│   ├── flac.c
│   ├── ogg.c
│   ├── mp4.c
│   ├── avi.c
│   └── mkv.c
│
└── arsip/                  # Archive formats
    ├── arsip.c
    ├── zip.c
    ├── tar.c
    ├── gz.c
    ├── bz2.c
    └── xz.c
```

---

## Direktori Berkas

Direktori `SUMBER/berkas/` berisi driver filesystem.

```
SUMBER/berkas/
│
├── vfs/                    # Virtual File System
│   ├── vfs.c
│   ├── filesystem.c
│   ├── mount.c
│   ├── inode.c
│   ├── dentry.c
│   ├── file.c
│   ├── superblock.c
│   └── namei.c
│
├── fat32/                  # FAT32 driver
│   ├── fat32.c
│   ├── fat32_boot.c
│   ├── fat32_fat.c
│   ├── fat32_dir.c
│   ├── fat32_file.c
│   ├── fat32_cluster.c
│   └── fat32_lfn.c
│
├── ntfs/                   # NTFS driver
│   ├── ntfs.c
│   ├── ntfs_boot.c
│   ├── ntfs_mft.c
│   ├── ntfs_attrib.c
│   ├── ntfs_dir.c
│   ├── ntfs_file.c
│   ├── ntfs_index.c
│   └── ntfs_compress.c
│
├── ext2/                   # ext2 driver
│   ├── ext2.c
│   ├── ext2_super.c
│   ├── ext2_inode.c
│   ├── ext2_dir.c
│   ├── ext2_file.c
│   ├── ext2_group.c
│   └── ext2_bitmap.c
│
├── iso9660/                # ISO9660 driver
│   ├── iso9660.c
│   ├── iso9660_primary.c
│   ├── iso9660_dir.c
│   ├── iso9660_file.c
│   ├── rockridge.c
│   └── joliet.c
│
├── initramfs/              # initramfs driver
│   ├── initramfs.c
│   ├── cpio.c
│   └── initramfs_extract.c
│
└── pigurafs/               # Native filesystem
    ├── pigurafs.c
    ├── pigurafs_super.c
    ├── pigurafs_inode.c
    ├── pigurafs_dir.c
    ├── pigurafs_file.c
    ├── pigurafs_btree.c
    ├── pigurafs_extent.c
    └── pigurafs_journal.c
```

---

## Direktori Aplikasi

Direktori `SUMBER/aplikasi/` berisi aplikasi user-space.

```
SUMBER/aplikasi/
│
├── sistem/                 # System apps
│   ├── sistem.c
│   ├── init.c              # Init process
│   ├── shell.c             # Shell
│   ├── login.c             # Login manager
│   ├── pengaturan.c        # Settings
│   ├── profil.c            # User profile
│   └── tentang.c           # About dialog
│
├── utilitas/               # Utilities
│   ├── utilitas.c
│   ├── berkas_manager.c    # File manager
│   ├── terminal.c          # Terminal
│   ├── editor_teks.c       # Text editor
│   ├── kalkulator.c
│   ├── kalender.c
│   ├── jam.c
│   ├── pencarian.c
│   ├── partisi.c
│   └── proses_monitor.c
│
├── grafis/                 # Graphics apps
│   ├── grafis.c
│   ├── editor_gambar.c
│   ├── penampil_gambar.c
│   ├── screenshot.c
│   ├── pengambil_warna.c
│   └── font_viewer.c
│
├── kantor/                 # Office apps
│   ├── kantor.c
│   ├── editor_dokumen.c
│   ├── spreadsheet.c
│   ├── presentasi.c
│   ├── penampil_teks.c
│   ├── pdf_reader.c
│   ├── catatan.c
│   └── kalender_app.c
│
└── jaringan/               # Network apps
    ├── jaringan.c
    ├── browser.c
    ├── email.c
    ├── chat.c
    ├── ftp.c
    ├── ssh.c
    ├── ping.c
    ├── netstat.c
    └── wifi_manager.c
```

---

## Direktori ALAT

Direktori `ALAT/` berisi peralatan build dan skrip.

```
ALAT/
├── build.sh                # Skrip build utama
├── clean.sh                # Skrip pembersihan
├── debug.sh                # Skrip debugging
├── qemu_run.sh             # Skrip QEMU
├── create_image.sh         # Skrip pembuatan image
├── linker_x86.ld           # Linker script x86
├── linker_x86_64.ld        # Linker script x86_64
├── linker_arm.ld           # Linker script ARM
├── linker_armv7.ld         # Linker script ARMv7
└── linker_arm64.ld         # Linker script ARM64
```

---

## Direktori DOKUMENTASI

Direktori `DOKUMENTASI/` berisi dokumentasi proyek.

```
DOKUMENTASI/
├── ARSITEKTUR.md           # Dokumentasi arsitektur
├── STRUKTUR.md             # Struktur direktori (file ini)
├── BUILD.md                # Panduan build
├── API.md                  # API reference
├── FRAMEBUFFER.md          # Sistem grafis
├── IC_DETECTION.md         # Sistem IC Detection
├── IMPLEMENTASI.md         # Status implementasi dan roadmap
└── KONTRIBUSI.md           # Panduan kontribusi
```

---

## Ringkasan

Struktur direktori Pigura OS dirancang dengan prinsip:

1. **Modular** - Setiap komponen terpisah dengan jelas
2. **Terorganisir** - Pembagian berdasarkan fungsi
3. **Skalabel** - Mudah menambah komponen baru
4. **Maintainable** - Mudah dipelihara dan dikembangkan
5. **Terintegrasi** - Graphics layer all-in-one di `pigura/`

### Inovasi Utama

| Aspek | Pendekatan Pigura | Keuntungan |
|-------|-------------------|------------|
| Graphics Layer | All-in-One (`pigura/`) | ~200 KB vs ~500 MB |
| Driver System | IC Detection | ~600 KB vs ~50 MB |
| LibC | Pigura C90 (bounded) | ~160 fungsi vs ~3000+ |
| **Total** | **~6-7 MB** | **99% lebih kecil** |

Total struktur ini mencakup sekitar **350 file** dengan estimasi ukuran **6-7 MB** - jauh lebih kecil dari sistem operasi modern pada umumnya.
