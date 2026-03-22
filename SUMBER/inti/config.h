/*
 * PIGURA OS - CONFIG.H
 * =====================
 * Konfigurasi build untuk kernel Pigura OS.
 *
 * Berkas ini berisi definisi konfigurasi yang mengontrol fitur-fitur
 * kernel yang dikompilasi. Konfigurasi dapat diatur melalui flag
 * compiler atau diedit langsung.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua konfigurasi didefinisikan secara eksplisit
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#ifndef INTI_CONFIG_H
#define INTI_CONFIG_H

/*
 * ===========================================================================
 * KONFIGURASI ARSITEKTUR (ARCHITECTURE CONFIGURATION)
 * ===========================================================================
 * Deteksi dan konfigurasi arsitektur target.
 * Arsitektur harus didefinisikan melalui flag compiler:
 *   -DARSITEKTUR_X86     untuk Intel/AMD 32-bit
 *   -DARSITEKTUR_X86_64  untuk Intel/AMD 64-bit
 *   -DARSITEKTUR_ARM32   untuk ARM 32-bit
 *   -DARSITEKTUR_ARM64   untuk ARM 64-bit (AArch64)
 */

/* Validasi: pastikan salah satu arsitektur didefinisikan */
#if defined(ARSITEKTUR_X86)
    #define NAMA_ARSITEKTUR "x86"
    #define NAMA_ARSITEKTUR_LENGKAP "x86 (32-bit)"
    #define LEBAR_BIT 32
    #define PAGING_32BIT 1
    #define PAGING_64BIT 0
    #define SUPPORT_PAE 0
    #define SUPPORT_SEGMENTASI 1

#elif defined(ARSITEKTUR_X86_64)
    #define NAMA_ARSITEKTUR "x86_64"
    #define NAMA_ARSITEKTUR_LENGKAP "x86_64 (64-bit)"
    #define LEBAR_BIT 64
    #define PAGING_32BIT 0
    #define PAGING_64BIT 1
    #define SUPPORT_LONG_MODE 1
    #define SUPPORT_SEGMENTASI 0

#elif defined(ARSITEKTUR_ARM32)
    #define NAMA_ARSITEKTUR "arm"
    #define NAMA_ARSITEKTUR_LENGKAP "ARM (32-bit)"
    #define LEBAR_BIT 32
    #define PAGING_32BIT 1
    #define SUPPORT_MMU_ARM 1
    #define SUPPORT_VFP 1

#elif defined(ARSITEKTUR_ARM64)
    #define NAMA_ARSITEKTUR "arm64"
    #define NAMA_ARSITEKTUR_LENGKAP "ARM64 (AArch64)"
    #define LEBAR_BIT 64
    #define PAGING_64BIT 1
    #define SUPPORT_VFP 1
    #define SUPPORT_NEON 1

#else
    /* Default ke x86 jika tidak didefinisikan (untuk backward compat) */
    #define ARSITEKTUR_X86 1
    #define NAMA_ARSITEKTUR "x86"
    #define NAMA_ARSITEKTUR_LENGKAP "x86 (32-bit)"
    #define LEBAR_BIT 32
    #define PAGING_32BIT 1
    #define PAGING_64BIT 0
    #define SUPPORT_SEGMENTASI 1
#endif

/*
 * ===========================================================================
 * VERSI KERNEL (KERNEL VERSION)
 * ===========================================================================
 * Informasi versi kernel Pigura OS.
 */

#define PIGURA_VERSI_MAJOR 1
#define PIGURA_VERSI_MINOR 0
#define PIGURA_VERSI_PATCH 0
#define PIGURA_VERSI_BUILD 0

#define PIGURA_VERSI_STRING "1.0.0"
#define PIGURA_NAMA "Pigura OS"
#define PIGURA_JULUKAN "Bingkai Digital"
#define PIGURA_HOMEPAGE "https://pigura.os"

/* Versi numerik untuk perbandingan */
#define PIGURA_VERSI_NUM \
    ((PIGURA_VERSI_MAJOR * 10000) + \
     (PIGURA_VERSI_MINOR * 100) + \
     PIGURA_VERSI_PATCH)

/*
 * ===========================================================================
 * KONFIGURASI DEBUG (DEBUG CONFIGURATION)
 * ===========================================================================
 * Pengaturan mode debug untuk pengembangan dan testing.
 */

/* Aktifkan debug mode */
#ifndef DEBUG
#define DEBUG 1
#endif

/* Aktifkan assertion */
#ifndef DEBUG_ASSERT
#define DEBUG_ASSERT 1
#endif

/* Aktifkan trace log */
#ifndef DEBUG_TRACE
#define DEBUG_TRACE 0
#endif

/* Aktifkan memory debug (detect leaks, corruption) */
#ifndef DEBUG_MEMORI
#define DEBUG_MEMORI 0
#endif

/* Aktifkan performance profiling */
#ifndef DEBUG_PROFILING
#define DEBUG_PROFILING 0
#endif

/* Level log:
 * 0 = Tidak ada log
 * 1 = Error saja
 * 2 = Error + Warning
 * 3 = Error + Warning + Info
 * 4 = Semua termasuk debug
 */
#ifndef CONFIG_LOG_LEVEL
#define CONFIG_LOG_LEVEL 3
#endif

/* Output log ke serial port */
#ifndef CONFIG_LOG_SERIAL
#define CONFIG_LOG_SERIAL 1
#endif

/* Output log ke framebuffer */
#ifndef CONFIG_LOG_FRAMEBUFFER
#define CONFIG_LOG_FRAMEBUFFER 1
#endif

/* Output log ke buffer memori */
#ifndef CONFIG_LOG_BUFFER
#define CONFIG_LOG_BUFFER 1
#endif

/* Ukuran buffer log */
#define CONFIG_LOG_BUFFER_UKURAN (16UL * 1024UL)

/*
 * ===========================================================================
 * KONFIGURASI MEMORI (MEMORY CONFIGURATION)
 * ===========================================================================
 * Pengaturan alokasi dan manajemen memori.
 */

/* Ukuran halaman (bytes) */
#define CONFIG_UKURAN_HALAMAN 4096UL

/* Ukuran minimum heap kernel (bytes) */
#define CONFIG_HEAP_MINIMAL (1UL * 1024UL * 1024UL)

/* Ukuran maksimum heap kernel (bytes), 0 = unlimited */
#define CONFIG_HEAP_MAKSIMAL 0

/* Algoritma alokasi memori fisik:
 * 0 = first-fit (cepat, mungkin fragmentasi)
 * 1 = best-fit (efisien, lebih lambat)
 * 2 = buddy system (cepat, overhead besar)
 */
#define CONFIG_ALOKASI_ALGORITMA 0

/* Aktifkan memory pooling */
#define CONFIG_MEMORY_POOL 1

/* Ukuran pool kecil (untuk alokasi < 128 bytes) */
#define CONFIG_POOL_KECIL_UKURAN 128
#define CONFIG_POOL_KECIL_JUMLAH 1024

/* Ukuran pool sedang (untuk alokasi < 1 KB) */
#define CONFIG_POOL_SEDANG_UKURAN 1024
#define CONFIG_POOL_SEDANG_JUMLAH 512

/* Ukuran pool besar (untuk alokasi < 4 KB) */
#define CONFIG_POOL_BESAR_UKURAN 4096
#define CONFIG_POOL_BESAR_JUMLAH 256

/* Aktifkan DMA buffer */
#define CONFIG_DMA_BUFFER 1

/* Ukuran DMA buffer pool */
#define CONFIG_DMA_POOL_UKURAN (16UL * 1024UL)

/* Aktifkan guard page untuk stack */
#define CONFIG_STACK_GUARD 1

/* Aktifkan memory poisoning (debug) */
#if DEBUG_MEMORI
#define CONFIG_MEMORI_POISON 1
#define MEMORI_POISON_ALLOC 0xAA
#define MEMORI_POISON_FREE 0xBB
#else
#define CONFIG_MEMORI_POISON 0
#endif

/*
 * ===========================================================================
 * KONFIGURASI PROSES (PROCESS CONFIGURATION)
 * ===========================================================================
 * Pengaturan manajemen proses dan thread.
 */

/* Maksimum jumlah proses */
#define CONFIG_MAKS_PROSES 256

/* Maksimum thread per proses */
#define CONFIG_MAKS_THREAD 16

/* Maksimum file descriptor per proses */
#define CONFIG_MAKS_FD 256

/* Maksimum open files system-wide */
#define CONFIG_MAKS_OPEN_FILES 4096

/* Ukuran stack kernel (bytes) */
#define CONFIG_STACK_KERNEL (8UL * 1024UL)

/* Ukuran stack user (bytes) */
#define CONFIG_STACK_USER (1UL * 1024UL * 1024UL)

/* Ukuran environment per proses */
#define CONFIG_ENV_SIZE 4096

/* Maksimum argumen */
#define CONFIG_MAKS_ARGC 32
#define CONFIG_MAKS_ARGV_LEN 256

/* Quantum scheduler (ticks) */
#define CONFIG_SCHEDULER_QUANTUM 10

/* Aktifkan priority scheduling */
#define CONFIG_PRIORITY_SCHED 1

/* Aktifkan preemptive scheduling */
#define CONFIG_PREEMPTIVE 1

/* Aktifkan SMP (multi-core) support */
#define CONFIG_SMP 0

/* Maksimum CPU core */
#define CONFIG_MAKS_CPU 8

/* Aktifkan CPU affinity */
#define CONFIG_CPU_AFFINITY 0

/*
 * ===========================================================================
 * KONFIGURASI FILESYSTEM (FILESYSTEM CONFIGURATION)
 * ===========================================================================
 * Pengaturan dukungan filesystem.
 */

/* Aktifkan VFS */
#define CONFIG_VFS 1

/* Dukungan FAT12/16/32 */
#define CONFIG_FS_FAT 1
#define CONFIG_FS_FAT_WRITE 1
#define CONFIG_FS_FAT_LFN 1

/* Dukungan NTFS */
#define CONFIG_FS_NTFS 1
#define CONFIG_FS_NTFS_WRITE 0

/* Dukungan ext2 */
#define CONFIG_FS_EXT2 1
#define CONFIG_FS_EXT2_WRITE 1

/* Dukungan ext3 (read-only) */
#define CONFIG_FS_EXT3_READ 1

/* Dukungan ext4 (read-only) */
#define CONFIG_FS_EXT4_READ 0

/* Dukungan ISO9660 */
#define CONFIG_FS_ISO9660 1

/* Dukungan RockRidge extension */
#define CONFIG_FS_ROCKRIDGE 1

/* Dukungan Joliet extension */
#define CONFIG_FS_JOLIET 1

/* Dukungan UDF */
#define CONFIG_FS_UDF 0

/* Dukungan initramfs */
#define CONFIG_FS_INITRAMFS 1

/* Dukungan PiguraFS (native) */
#define CONFIG_FS_PIGURAFS 1
#define CONFIG_FS_PIGURAFS_JOURNAL 1

/* Dukungan devfs */
#define CONFIG_FS_DEVFS 1

/* Dukungan procfs */
#define CONFIG_FS_PROCFS 1

/* Dukungan sysfs */
#define CONFIG_FS_SYSFS 0

/* Dukungan tmpfs */
#define CONFIG_FS_TMPFS 1

/* Maksimum mount point */
#define CONFIG_MAKS_MOUNT 16

/* Ukuran buffer cache */
#define CONFIG_BUFFER_CACHE (4UL * 1024UL * 1024UL)

/* Maksimum path length */
#define CONFIG_PATH_MAX 256

/* Maksimum nama file */
#define CONFIG_NAME_MAX 255

/*
 * ===========================================================================
 * KONFIGURASI DEVICE DRIVER (DEVICE DRIVER CONFIGURATION)
 * ===========================================================================
 * Pengaturan driver perangkat.
 */

/* Aktifkan IC Detection System */
#define CONFIG_IC_DETECTION 1

/* Dukungan framebuffer */
#define CONFIG_FRAMEBUFFER 1

/* Dukungan VBE (VESA BIOS Extensions) */
#define CONFIG_VBE 1

/* Dukungan UEFI GOP */
#define CONFIG_UEFI_GOP 1

/* Dukungan GPU acceleration */
#define CONFIG_GPU_ACCEL 0

/* Resolusi default */
#define CONFIG_LEBAR_LAYAR 1024
#define CONFIG_TINGGI_LAYAR 768
#define CONFIG_BIT_DEPTH 32

/* Dukungan keyboard */
#define CONFIG_KEYBOARD 1

/* Dukungan mouse */
#define CONFIG_MOUSE 1

/* Dukungan touchscreen */
#define CONFIG_TOUCHSCREEN 0

/* Dukungan gamepad/joystick */
#define CONFIG_GAMEPAD 0

/* Dukungan storage */
#define CONFIG_STORAGE 1

/* Dukungan ATA/IDE */
#define CONFIG_ATA 1

/* Dukungan AHCI (SATA) */
#define CONFIG_AHCI 1

/* Dukungan NVMe */
#define CONFIG_NVME 0

/* Dukungan USB mass storage */
#define CONFIG_USB_STORAGE 1

/* Dukungan SD card */
#define CONFIG_SD_CARD 1

/* Dukungan floppy */
#define CONFIG_FLOPPY 0

/* Dukungan audio */
#define CONFIG_AUDIO 0

/* Dukungan network */
#define CONFIG_NETWORK 0

/* Dukungan WiFi */
#define CONFIG_WIFI 0

/* Dukungan Bluetooth */
#define CONFIG_BLUETOOTH 0

/* Dukungan USB HID */
#define CONFIG_USB_HID 1

/* Dukungan serial port */
#define CONFIG_SERIAL 1

/*
 * ===========================================================================
 * KONFIGURASI GRAFIS (GRAPHICS CONFIGURATION)
 * ===========================================================================
 * Pengaturan sistem grafis.
 */

/* Aktifkan LibPigura */
#define CONFIG_LIBPIGURA 1

/* Aktifkan Dekor compositor */
#define CONFIG_DEKOR 1

/* Aktifkan double buffering */
#define CONFIG_DOUBLE_BUFFER 1

/* Aktifkan efek visual */
#define CONFIG_EFEK_VISUAL 0

/* Aktifkan transparansi */
#define CONFIG_TRANSPARANSI 0

/* Aktifkan shadow */
#define CONFIG_SHADOW 0

/* Aktifkan animasi */
#define CONFIG_ANIMASI 0

/* Font default */
#define CONFIG_FONT_DEFAULT "default"

/* Ukuran font default */
#define CONFIG_FONT_SIZE 12

/*
 * ===========================================================================
 * KONFIGURASI KEAMANAN (SECURITY CONFIGURATION)
 * ===========================================================================
 * Pengaturan keamanan sistem.
 */

/* Aktifkan user isolation */
#define CONFIG_USER_ISOLATION 1

/* Aktifkan address space randomization */
#define CONFIG_ASLR 0

/* Aktifkan stack canary */
#define CONFIG_STACK_CANARY 1

/* Aktifkan NX bit (no-execute) */
#define CONFIG_NX_BIT 0

/* Aktifkan privilege separation */
#define CONFIG_PRIVILEGE_SEP 1

/* Aktifkan chroot */
#define CONFIG_CHROOT 1

/* Aktifkan capabilities */
#define CONFIG_CAPABILITIES 0

/* Password hashing algorithm */
#define CONFIG_PASSWORD_HASH "sha256"

/*
 * ===========================================================================
 * KONFIGURASI POWER MANAGEMENT (POWER MANAGEMENT CONFIGURATION)
 * ===========================================================================
 * Pengaturan manajemen daya.
 */

/* Aktifkan ACPI */
#define CONFIG_ACPI 1

/* Aktifkan APM (untuk sistem lama) */
#define CONFIG_APM 0

/* Aktifkan sleep state */
#define CONFIG_SLEEP 0

/* Aktifkan hibernate */
#define CONFIG_HIBERNATE 0

/* Aktifkan CPU frequency scaling */
#define CONFIG_CPU_FREQ 0

/*
 * ===========================================================================
 * KONFIGURASI BOOT (BOOT CONFIGURATION)
 * ===========================================================================
 * Pengaturan proses boot.
 */

/* Boot method: 0 = BIOS, 1 = UEFI */
#define CONFIG_BOOT_METHOD 0

/* Multiboot version: 1 atau 2 */
#define CONFIG_MULTIBOOT_VERSI 1

/* Timeout boot menu (detik) */
#define CONFIG_BOOT_TIMEOUT 5

/* Default boot entry */
#define CONFIG_BOOT_DEFAULT 0

/* Aktifkan boot splash */
#define CONFIG_BOOT_SPLASH 1

/* Aktifkan verbose boot */
#define CONFIG_BOOT_VERBOSE 1

/* Kernel command line default */
#define CONFIG_CMDLINE_DEFAULT ""

/*
 * ===========================================================================
 * KONFIGURASI KERNEL (KERNEL CONFIGURATION)
 * ===========================================================================
 * Pengaturan umum kernel.
 */

/* Nama kernel */
#define CONFIG_NAMA_KERNEL "pigura"

/* Hostname default */
#define CONFIG_HOSTNAME_DEFAULT "pigura"

/* Aktifkan kernel modules */
#define CONFIG_MODULES 0

/* Aktifkan hotplug */
#define CONFIG_HOTPLUG 0

/* Aktifkan kexec */
#define CONFIG_KEXEC 0

/* Ukuran kernel log buffer */
#define CONFIG_LOG_BUF_SIZE (16UL * 1024UL)

/* Aktifkan magic sysrq */
#define CONFIG_MAGIC_SYSRQ 0

/* Aktifkan kernel debugger */
#define CONFIG_KDB 0

/*
 * ===========================================================================
 * KONFIGURASI LOKALISASI (LOCALIZATION CONFIGURATION)
 * ===========================================================================
 * Pengaturan bahasa dan lokal.
 */

/* Bahasa default */
#define CONFIG_BAHASA "id_ID"

/* Encoding */
#define CONFIG_ENCODING "UTF-8"

/* Timezone default */
#define CONFIG_TIMEZONE "Asia/Jakarta"

/* Format tanggal: 0 = DD/MM/YYYY, 1 = MM/DD/YYYY, 2 = YYYY-MM-DD */
#define CONFIG_FORMAT_TANGGAL 0

/* Format waktu: 0 = 24 jam, 1 = 12 jam */
#define CONFIG_FORMAT_WAKTU 0

/* Hari pertama minggu: 0 = Minggu, 1 = Senin */
#define CONFIG_HARI_PERTAMA 1

/*
 * ===========================================================================
 * KONFIGURASI TIMER (TIMER CONFIGURATION)
 * ===========================================================================
 * Pengaturan timer sistem.
 */

/* Frekuensi timer (Hz) */
#define CONFIG_TIMER_FREQ 100

/* Milidetik per tick */
#define CONFIG_MS_PER_TICK (1000 / CONFIG_TIMER_FREQ)

/* Aktifkan high-resolution timer */
#define CONFIG_HRTIMER 0

/* Aktifkan timer slack */
#define CONFIG_TIMER_SLACK 0

/*
 * ===========================================================================
 * VALIDASI KONFIGURASI (CONFIGURATION VALIDATION)
 * ===========================================================================
 * Validasi konsistensi konfigurasi.
 */

/* Pastikan konfigurasi proses valid */
#if CONFIG_MAKS_PROSES < 2
#error "CONFIG_MAKS_PROSES harus minimal 2 (kernel + init)"
#endif

/* Pastikan quantum scheduler valid */
#if CONFIG_SCHEDULER_QUANTUM < 1
#error "CONFIG_SCHEDULER_QUANTUM harus minimal 1"
#endif

/* Pastikan ukuran halaman valid */
#if CONFIG_UKURAN_HALAMAN != 4096
#error "CONFIG_UKURAN_HALAMAN harus 4096"
#endif

/* Pastikan timer frequency valid */
#if CONFIG_TIMER_FREQ < 1 || CONFIG_TIMER_FREQ > 1000
#error "CONFIG_TIMER_FREQ harus antara 1 dan 1000"
#endif

/*
 * ===========================================================================
 * MAKRO HELPER KONFIGURASI (CONFIGURATION HELPER MACROS)
 * ===========================================================================
 * Makro bantuan untuk pengecekan konfigurasi.
 */

/* Cek apakah fitur enabled */
#define KONFIG_ENABLED(x) ((x) != 0)
#define KONFIG_DISABLED(x) ((x) == 0)

/* Helper untuk kondisional compile-time */
#if DEBUG
#define IF_DEBUG(stmt) stmt
#else
#define IF_DEBUG(stmt)
#endif

#if CONFIG_SMP
#define IF_SMP(stmt) stmt
#else
#define IF_SMP(stmt)
#endif

#if CONFIG_NETWORK
#define IF_NETWORK(stmt) stmt
#else
#define IF_NETWORK(stmt)
#endif

#if CONFIG_AUDIO
#define IF_AUDIO(stmt) stmt
#else
#define IF_AUDIO(stmt)
#endif

#if CONFIG_FRAMEBUFFER
#define IF_FRAMEBUFFER(stmt) stmt
#else
#define IF_FRAMEBUFFER(stmt)
#endif

/*
 * ===========================================================================
 * MAKRO TRACE DAN LOG (TRACE AND LOG MACROS)
 * ===========================================================================
 * Makro untuk debugging dan tracing.
 */

#if DEBUG_TRACE
#define TRACE_ENTER(func) \
    kernel_printk(LOG_DEBUG, "[TRACE] Enter: %s\n", func)
#define TRACE_EXIT(func) \
    kernel_printk(LOG_DEBUG, "[TRACE] Exit: %s\n", func)
#else
#define TRACE_ENTER(func) ((void)0)
#define TRACE_EXIT(func) ((void)0)
#endif

/* Makro untuk menandai fungsi yang digunakan */
#define DIGUNAKAN __attribute__((used))

/* Makro untuk menandai parameter tidak digunakan */
#define TIDAK_DIGUNAKAN_PARAM(param) ((void)(param))

/* Makro untuk constructor/destructor */
#define KONSTRUKTOR __attribute__((constructor))
#define DESTRUKTOR __attribute__((destructor))

/* Makro untuk aligned attribute */
#define ALIGNED(n) __attribute__((aligned(n)))

/* Makro untuk packed attribute */
#define PACKED __attribute__((packed))

/* Makro untuk section */
#define SECTION(s) __attribute__((section(s)))

/* Makro untuk weak symbol */
#define LEMAH __attribute__((weak))

/* Makro untuk noreturn */
#define TIDAK_RETURN __attribute__((noreturn))

/* Makro untuk pure function */
#define MURNI __attribute__((pure))

/* Makro untuk const function */
#define KONSTAN_FUNC __attribute__((const))

/* Makro untuk inline hint */
#define INLINE_HINT __attribute__((always_inline))

/* Makro untuk noinline */
#define NOINLINE __attribute__((noinline))

/* Makro untuk deprecated */
#define USANG __attribute__((deprecated))

/* Makro untuk fallthrough */
#define LANJUT __attribute__((fallthrough))

#endif /* INTI_CONFIG_H */
