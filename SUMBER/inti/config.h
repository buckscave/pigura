/*
 * PIGURA OS - CONFIG.H
 * --------------------
 * Konfigurasi build untuk kernel Pigura OS.
 *
 * Berkas ini berisi definisi konfigurasi yang mengontrol fitur-fitur
 * kernel yang dikompilasi. Konfigurasi dapat diatur melalui flag
 * compiler atau diedit langsung.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef INTI_CONFIG_H
#define INTI_CONFIG_H

/*
 * ============================================================================
 * KONFIGURASI ARSITEKTUR (ARCHITECTURE CONFIGURATION)
 * ============================================================================
 * Konfigurasi arsitektur target. Pilih satu dari opsi yang tersedia.
 */

/* Arsitektur yang didukung:
 * - ARSITEKTUR_X86    : Intel/AMD 32-bit
 * - ARSITEKTUR_X86_64 : Intel/AMD 64-bit
 * - ARSITEKTUR_ARM    : ARM 32-bit generik
 * - ARSITEKTUR_ARMV7  : ARM Cortex-A series
 */

/* Default ke x86 jika tidak didefinisikan */
#if !defined(ARSITEKTUR_X86) && !defined(ARSITEKTUR_X86_64) && \
    !defined(ARSITEKTUR_ARM) && !defined(ARSITEKTUR_ARMV7)
    #define ARSITEKTUR_X86      1
#endif

/* Fitur arsitektur berdasarkan pilihan */
#ifdef ARSITEKTUR_X86
    #define NAMA_ARSITEKTUR     "x86 (32-bit)"
    #define LEBAR_BIT           32
    #define PAGING_32BIT        1
    #define PAGING_64BIT        0
    #define SUPPORT_PAE         0
#endif

#ifdef ARSITEKTUR_X86_64
    #define NAMA_ARSITEKTUR     "x86_64 (64-bit)"
    #define LEBAR_BIT           64
    #define PAGING_32BIT        0
    #define PAGING_64BIT        1
    #define SUPPORT_LONG_MODE   1
#endif

#ifdef ARSITEKTUR_ARM
    #define NAMA_ARSITEKTUR     "ARM (32-bit)"
    #define LEBAR_BIT           32
    #define PAGING_32BIT        1
    #define SUPPORT_MMU_ARM     1
#endif

#ifdef ARSITEKTUR_ARMV7
    #define NAMA_ARSITEKTUR     "ARMv7 (Cortex-A)"
    #define LEBAR_BIT           32
    #define PAGING_32BIT        1
    #define SUPPORT_VFP         1
    #define SUPPORT_NEON        1
#endif

/*
 * ============================================================================
 * KONFIGURASI DEBUG (DEBUG CONFIGURATION)
 * ============================================================================
 * Aktifkan mode debug untuk pengembangan dan testing.
 */

/* Aktifkan debug mode */
#ifndef DEBUG
    #define DEBUG               1
#endif

/* Aktifkan assertion */
#ifndef DEBUG_ASSERT
    #define DEBUG_ASSERT        1
#endif

/* Aktifkan trace log */
#ifndef DEBUG_TRACE
    #define DEBUG_TRACE         0
#endif

/* Aktifkan memory debug (detect leaks, corruption) */
#ifndef DEBUG_MEMORI
    #define DEBUG_MEMORI        0
#endif

/* Aktifkan performance profiling */
#ifndef DEBUG_PROFILING
    #define DEBUG_PROFILING     0
#endif

/* Level log:
 * 0 = Tidak ada log
 * 1 = Error saja
 * 2 = Error + Warning
 * 3 = Error + Warning + Info
 * 4 = Semua termasuk debug
 */
#ifndef LOG_LEVEL
    #define LOG_LEVEL           3
#endif

/* Output log ke serial port */
#ifndef LOG_SERIAL
    #define LOG_SERIAL          1
#endif

/* Output log ke framebuffer */
#ifndef LOG_FRAMEBUFFER
    #define LOG_FRAMEBUFFER     1
#endif

/*
 * ============================================================================
 * KONFIGURASI MEMORI (MEMORY CONFIGURATION)
 * ============================================================================
 * Pengaturan alokasi dan manajemen memori.
 */

/* Ukuran halaman (bytes) */
#define CONFIG_UKURAN_HALAMAN       4096

/* Ukuran minimum heap kernel (bytes) */
#define CONFIG_HEAP_MINIMAL         (1UL * 1024UL * 1024UL)  /* 1 MB */

/* Ukuran maksimum heap kernel (bytes), 0 = unlimited */
#define CONFIG_HEAP_MAKSIMAL        0

/* Alokasi memori fisik: 0 = first-fit, 1 = best-fit, 2 = buddy */
#define CONFIG_ALOKASI_ALGORITMA    0

/* Aktifkan memory pooling */
#define CONFIG_MEMORY_POOL          1

/* Ukuran pool kecil (untuk alokasi < 128 bytes) */
#define CONFIG_POOL_KECIL_UKURAN    128
#define CONFIG_POOL_KECIL_JUMLAH    1024

/* Ukuran pool sedang (untuk alokasi < 1 KB) */
#define CONFIG_POOL_SEDANG_UKURAN   1024
#define CONFIG_POOL_SEDANG_JUMLAH   512

/* Aktifkan DMA buffer */
#define CONFIG_DMA_BUFFER           1

/* Ukuran DMA buffer pool */
#define CONFIG_DMA_POOL_UKURAN      (16UL * 1024UL)  /* 16 KB */

/*
 * ============================================================================
 * KONFIGURASI PROSES (PROCESS CONFIGURATION)
 * ============================================================================
 * Pengaturan manajemen proses dan thread.
 */

/* Maksimum jumlah proses */
#define CONFIG_MAKS_PROSES          256

/* Maksimum thread per proses */
#define CONFIG_MAKS_THREAD          16

/* Maksimum file descriptor per proses */
#define CONFIG_MAKS_FD              256

/* Ukuran stack kernel (bytes) */
#define CONFIG_STACK_KERNEL         (8UL * 1024UL)  /* 8 KB */

/* Ukuran stack user (bytes) */
#define CONFIG_STACK_USER           (1UL * 1024UL * 1024UL)  /* 1 MB */

/* Quantum scheduler (ticks) */
#define CONFIG_SCHEDULER_QUANTUM    10

/* Aktifkan priority scheduling */
#define CONFIG_PRIORITY_SCHED       1

/* Aktifkan preemptive scheduling */
#define CONFIG_PREEMPTIVE           1

/* Aktifkan SMP (multi-core) support */
#define CONFIG_SMP                  0

/* Maksimum CPU core */
#define CONFIG_MAKS_CPU             8

/*
 * ============================================================================
 * KONFIGURASI FILESYSTEM (FILESYSTEM CONFIGURATION)
 * ============================================================================
 * Pengaturan dukungan filesystem.
 */

/* Aktifkan VFS */
#define CONFIG_VFS                  1

/* Dukungan FAT32 */
#define CONFIG_FS_FAT32             1
#define CONFIG_FS_FAT32_WRITE       1

/* Dukungan NTFS */
#define CONFIG_FS_NTFS              1
#define CONFIG_FS_NTFS_WRITE        0  /* Read-only untuk keamanan */

/* Dukungan ext2 */
#define CONFIG_FS_EXT2              1
#define CONFIG_FS_EXT2_WRITE        1

/* Dukungan ext3/ext4 (read-only) */
#define CONFIG_FS_EXT3_READ         1
#define CONFIG_FS_EXT4_READ         0

/* Dukungan ISO9660 */
#define CONFIG_FS_ISO9660           1

/* Dukungan RockRidge extension */
#define CONFIG_FS_ROCKRIDGE         1

/* Dukungan Joliet extension */
#define CONFIG_FS_JOLIET            1

/* Dukungan initramfs */
#define CONFIG_FS_INITRAMFS         1

/* Dukungan PiguraFS (native) */
#define CONFIG_FS_PIGURAFS          1
#define CONFIG_FS_PIGURAFS_JOURNAL  1

/* Maksimum mount point */
#define CONFIG_MAKS_MOUNT           16

/* Cache buffer size */
#define CONFIG_BUFFER_CACHE         (4UL * 1024UL * 1024UL)  /* 4 MB */

/*
 * ============================================================================
 * KONFIGURASI DEVICE DRIVER (DEVICE DRIVER CONFIGURATION)
 * ============================================================================
 * Pengaturan driver perangkat.
 */

/* Aktifkan IC Detection System */
#define CONFIG_IC_DETECTION         1

/* Dukungan framebuffer */
#define CONFIG_FRAMEBUFFER          1

/* Dukungan VBE (VESA BIOS Extensions) */
#define CONFIG_VBE                  1

/* Dukungan UEFI GOP */
#define CONFIG_UEFI_GOP             1

/* Dukungan GPU acceleration */
#define CONFIG_GPU_ACCEL            0

/* Dukungan keyboard */
#define CONFIG_KEYBOARD             1

/* Dukungan mouse */
#define CONFIG_MOUSE                1

/* Dukungan touchscreen */
#define CONFIG_TOUCHSCREEN          0

/* Dukungan storage */
#define CONFIG_STORAGE              1

/* Dukungan ATA/IDE */
#define CONFIG_ATA                  1

/* Dukungan AHCI (SATA) */
#define CONFIG_AHCI                 1

/* Dukungan NVMe */
#define CONFIG_NVME                 0

/* Dukungan USB mass storage */
#define CONFIG_USB_STORAGE          1

/* Dukungan SD card */
#define CONFIG_SD_CARD              1

/* Dukungan audio */
#define CONFIG_AUDIO                0

/* Dukungan network */
#define CONFIG_NETWORK              0

/* Dukungan WiFi */
#define CONFIG_WIFI                 0

/*
 * ============================================================================
 * KONFIGURASI GRAFIS (GRAPHICS CONFIGURATION)
 * ============================================================================
 * Pengaturan sistem grafis.
 */

/* Aktifkan LibPigura */
#define CONFIG_LIBPIGURA            1

/* Aktifkan Dekor compositor */
#define CONFIG_DEKOR                1

/* Resolusi default */
#define CONFIG_LEBAR_LAYAR          1024
#define CONFIG_TINGGI_LAYAR         768

/* Bit depth default */
#define CONFIG_BIT_DEPTH            32

/* Aktifkan double buffering */
#define CONFIG_DOUBLE_BUFFER        1

/* Aktifkan efek visual */
#define CONFIG_EFEK_VISUAL          0

/* Aktifkan transparansi */
#define CONFIG_TRANSPARANSI         0

/* Aktifkan shadow */
#define CONFIG_SHADOW               0

/*
 * ============================================================================
 * KONFIGURASI FONT (FONT CONFIGURATION)
 * ============================================================================
 * Pengaturan sistem font.
 */

/* Font default */
#define CONFIG_FONT_DEFAULT         "font_default.pf"

/* Aktifkan bitmap font */
#define CONFIG_FONT_BITMAP          1

/* Aktifkan TTF font */
#define CONFIG_FONT_TTF             0

/* Ukuran font cache */
#define CONFIG_FONT_CACHE_SIZE      4096

/*
 * ============================================================================
 * KONFIGURASI APLIKASI (APPLICATION CONFIGURATION)
 * ============================================================================
 * Pengaturan aplikasi sistem.
 */

/* Aktifkan shell */
#define CONFIG_SHELL                1

/* Aktifkan init process */
#define CONFIG_INIT                 1

/* Aktifkan login */
#define CONFIG_LOGIN                1

/* Aktifkan file manager */
#define CONFIG_FILE_MANAGER         1

/* Aktifkan text editor */
#define CONFIG_TEXT_EDITOR          1

/* Aktifkan terminal */
#define CONFIG_TERMINAL             1

/* Aktifkan calculator */
#define CONFIG_CALCULATOR           1

/* Aktifkan image viewer */
#define CONFIG_IMAGE_VIEWER         1

/*
 * ============================================================================
 * KONFIGURASI KEAMANAN (SECURITY CONFIGURATION)
 * ============================================================================
 * Pengaturan keamanan sistem.
 */

/* Aktifkan user isolation */
#define CONFIG_USER_ISOLATION       1

/* Aktifkan address space randomization */
#define CONFIG_ASLR                 0

/* Aktifkan stack canary */
#define CONFIG_STACK_CANARY         1

/* Aktifkan NX bit (no-execute) */
#define CONFIG_NX_BIT               0

/* Aktifkan privilege separation */
#define CONFIG_PRIVILEGE_SEP        1

/*
 * ============================================================================
 * KONFIGURASI POWER MANAGEMENT (POWER MANAGEMENT CONFIGURATION)
 * ============================================================================
 * Pengaturan manajemen daya.
 */

/* Aktifkan ACPI */
#define CONFIG_ACPI                 1

/* Aktifkan APM (untuk sistem lama) */
#define CONFIG_APM                  0

/* Aktifkan sleep state */
#define CONFIG_SLEEP                0

/* Aktifkan hibernate */
#define CONFIG_HIBERNATE            0

/*
 * ============================================================================
 * KONFIGURASI BOOT (BOOT CONFIGURATION)
 * ============================================================================
 * Pengaturan proses boot.
 */

/* Boot method: 0 = BIOS, 1 = UEFI */
#define CONFIG_BOOT_METHOD          0

/* Multiboot version: 1 atau 2 */
#define CONFIG_MULTIBOOT_VERSI      1

/* Timeout boot menu (detik) */
#define CONFIG_BOOT_TIMEOUT         5

/* Default boot entry */
#define CONFIG_BOOT_DEFAULT         0

/* Aktifkan boot splash */
#define CONFIG_BOOT_SPLASH          1

/* Aktifkan verbose boot */
#define CONFIG_BOOT_VERBOSE         1

/*
 * ============================================================================
 * KONFIGURASI PERFORMA (PERFORMANCE CONFIGURATION)
 * ============================================================================
 * Pengaturan optimasi performa.
 */

/* Aktifkan cache optimization */
#define CONFIG_CACHE_OPT            1

/* Aktifkan prefetch */
#define CONFIG_PREFETCH             1

/* Aktifkan lazy allocation */
#define CONFIG_LAZY_ALLOC           1

/* Aktifkan zero-copy */
#define CONFIG_ZERO_COPY            0

/* Aktifkan SIMD optimization */
#define CONFIG_SIMD_OPT             0

/*
 * ============================================================================
 * KONFIGURASI LOKALISASI (LOCALIZATION CONFIGURATION)
 * ============================================================================
 * Pengaturan bahasa dan lokal.
 */

/* Bahasa default */
#define CONFIG_BAHASA               "id_ID"

/* Encoding */
#define CONFIG_ENCODING             "UTF-8"

/* Timezone default */
#define CONFIG_TIMEZONE             "Asia/Jakarta"

/* Format tanggal: 0 = DD/MM/YYYY, 1 = MM/DD/YYYY, 2 = YYYY-MM-DD */
#define CONFIG_FORMAT_TANGGAL       0

/* Format waktu: 0 = 24 jam, 1 = 12 jam */
#define CONFIG_FORMAT_WAKTU         0

/*
 * ============================================================================
 * KONFIGURASI KERNEL (KERNEL CONFIGURATION)
 * ============================================================================
 * Pengaturan umum kernel.
 */

/* Nama kernel */
#define CONFIG_NAMA_KERNEL          "pigura"

/* Versi kernel string */
#define CONFIG_VERSI_STRING         PIGURA_VERSI_STRING

/* Hostname default */
#define CONFIG_HOSTNAME_DEFAULT     "pigura"

/* Aktifkan kernel modules */
#define CONFIG_MODULES              0

/* Aktifkan hotplug */
#define CONFIG_HOTPLUG              0

/* Aktifkan kexec */
#define CONFIG_KEXEC                0

/* Ukuran kernel log buffer */
#define CONFIG_LOG_BUF_SIZE         (16UL * 1024UL)  /* 16 KB */

/* Aktifkan sysfs */
#define CONFIG_SYSFS                0

/* Aktifkan procfs */
#define CONFIG_PROCFS               1

/* Aktifkan devfs */
#define CONFIG_DEVFS                1

/*
 * ============================================================================
 * VALIDASI KONFIGURASI (CONFIGURATION VALIDATION)
 * ============================================================================
 * Validasi konsistensi konfigurasi.
 */

/* Pastikan salah satu arsitektur dipilih */
#if !defined(ARSITEKTUR_X86) && !defined(ARSITEKTUR_X86_64) && \
    !defined(ARSITEKTUR_ARM) && !defined(ARSITEKTUR_ARMV7)
    #error "Tidak ada arsitektur yang dipilih. Definisikan salah satu: ARSITEKTUR_X86, ARSITEKTUR_X86_64, ARSITEKTUR_ARM, atau ARSITEKTUR_ARMV7"
#endif

/* Pastikan konfigurasi memori valid */
#if CONFIG_UKURAN_HALAMAN != 4096
    #warning "Ukuran halaman yang tidak standar mungkin menyebabkan masalah kompatibilitas"
#endif

/* Pastikan konfigurasi proses valid */
#if CONFIG_MAKS_PROSES < 2
    #error "CONFIG_MAKS_PROSES harus minimal 2 (kernel + init)"
#endif

/* Pastikan quantum scheduler valid */
#if CONFIG_SCHEDULER_QUANTUM < 1
    #error "CONFIG_SCHEDULER_QUANTUM harus minimal 1"
#endif

/*
 * ============================================================================
 * MACRO HELPER UNTUK KONFIGURASI (CONFIGURATION HELPER MACROS)
 * ============================================================================
 */

/* Cek apakah fitur enabled */
#ifdef DEBUG
    #define IF_DEBUG(stmt) stmt
#else
    #define IF_DEBUG(stmt)
#endif

#ifdef CONFIG_SMP
    #define IF_SMP(stmt) stmt
#else
    #define IF_SMP(stmt)
#endif

#ifdef CONFIG_NETWORK
    #define IF_NETWORK(stmt) stmt
#else
    #define IF_NETWORK(stmt)
#endif

#ifdef CONFIG_AUDIO
    #define IF_AUDIO(stmt) stmt
#else
    #define IF_AUDIO(stmt)
#endif

/* Helper untuk kondisional compile-time */
#define KONFIG_ENABLED(x)       (x)
#define KONFIG_DISABLED(x)      (!(x))

#endif /* INTI_CONFIG_H */
