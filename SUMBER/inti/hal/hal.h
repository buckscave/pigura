/*
 * PIGURA OS - HAL.H
 * ==================
 * Header Hardware Abstraction Layer (HAL).
 *
 * HAL menyediakan interface yang portabel untuk operasi hardware,
 * memungkinkan kernel berjalan di berbagai arsitektur tanpa modifikasi.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi dideklarasikan secara eksplisit
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#ifndef INTI_HAL_HAL_H
#define INTI_HAL_HAL_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../types.h"
#include "../konstanta.h"

/*
 * ===========================================================================
 * KONSTANTA HAL (HAL CONSTANTS)
 * ===========================================================================
 */

/* Versi HAL */
#define HAL_VERSI_MAJOR 1
#define HAL_VERSI_MINOR 0
#define HAL_VERSI_PATCH 0

/* Magic number untuk validasi */
#define HAL_MAGIC 0x48414C00  /* "HAL\0" */

/* Status HAL */
#define HAL_STATUS_UNINITIALIZED 0
#define HAL_STATUS_INITIALIZING 1
#define HAL_STATUS_READY 2
#define HAL_STATUS_ERROR 3

/* CPU feature flags */
#define HAL_CPU_FEATURE_FPU 0x00000001
#define HAL_CPU_FEATURE_VME 0x00000002
#define HAL_CPU_FEATURE_DE 0x00000004
#define HAL_CPU_FEATURE_PSE 0x00000008
#define HAL_CPU_FEATURE_TSC 0x00000010
#define HAL_CPU_FEATURE_MSR 0x00000020
#define HAL_CPU_FEATURE_PAE 0x00000040
#define HAL_CPU_FEATURE_MCE 0x00000080
#define HAL_CPU_FEATURE_CX8 0x00000100
#define HAL_CPU_FEATURE_APIC 0x00000200
#define HAL_CPU_FEATURE_MTRR 0x00001000
#define HAL_CPU_FEATURE_PGE 0x00002000
#define HAL_CPU_FEATURE_MCA 0x00004000
#define HAL_CPU_FEATURE_CMOV 0x00008000
#define HAL_CPU_FEATURE_PAT 0x00010000
#define HAL_CPU_FEATURE_PSE36 0x00020000
#define HAL_CPU_FEATURE_PSN 0x00040000
#define HAL_CPU_FEATURE_CLFSH 0x00080000
#define HAL_CPU_FEATURE_DS 0x00200000
#define HAL_CPU_FEATURE_ACPI 0x00400000
#define HAL_CPU_FEATURE_MMX 0x00800000
#define HAL_CPU_FEATURE_FXSR 0x01000000
#define HAL_CPU_FEATURE_SSE 0x02000000
#define HAL_CPU_FEATURE_SSE2 0x04000000
#define HAL_CPU_FEATURE_SS 0x08000000
#define HAL_CPU_FEATURE_HTT 0x10000000
#define HAL_CPU_FEATURE_TM 0x20000000
#define HAL_CPU_FEATURE_PBE 0x80000000

/* Extended CPU features */
#define HAL_CPU_FEATURE_EXT_SSE3 0x00000001
#define HAL_CPU_FEATURE_EXT_PCLMUL 0x00000002
#define HAL_CPU_FEATURE_EXT_SSSE3 0x00000200
#define HAL_CPU_FEATURE_EXT_FMA 0x00001000
#define HAL_CPU_FEATURE_EXT_CX16 0x00002000
#define HAL_CPU_FEATURE_EXT_SSE41 0x00080000
#define HAL_CPU_FEATURE_EXT_SSE42 0x00100000
#define HAL_CPU_FEATURE_EXT_X2APIC 0x00200000
#define HAL_CPU_FEATURE_EXT_AVX 0x00400000
#define HAL_CPU_FEATURE_EXT_F16C 0x00800000
#define HAL_CPU_FEATURE_EXT_RDRAND 0x04000000

/*
 * ===========================================================================
 * STRUKTUR DATA HAL (HAL DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur informasi CPU */
typedef struct {
    tak_bertanda32_t id;            /* CPU ID */
    tak_bertanda32_t apic_id;       /* APIC ID */
    char vendor[13];                /* Vendor string (12 chars + null) */
    char brand[49];                 /* Brand string (48 chars + null) */
    tak_bertanda32_t family;        /* CPU family */
    tak_bertanda32_t model;         /* CPU model */
    tak_bertanda32_t stepping;      /* CPU stepping */
    tak_bertanda32_t features;      /* Feature flags (EDX) */
    tak_bertanda32_t features_ext;  /* Extended features (ECX) */
    tak_bertanda32_t cache_line;    /* Cache line size */
    tak_bertanda32_t cache_l1_inst; /* L1 instruction cache */
    tak_bertanda32_t cache_l1_data; /* L1 data cache */
    tak_bertanda32_t cache_l2;      /* L2 cache */
    tak_bertanda32_t cache_l3;      /* L3 cache */
    tak_bertanda32_t freq_khz;      /* Frequency in kHz */
    tak_bertanda8_t cores;          /* Number of cores */
    tak_bertanda8_t threads;        /* Number of threads */
    tak_bertanda8_t socket;         /* Socket number */
    tak_bertanda8_t reserved;
} hal_cpu_info_t;

/* Struktur informasi timer */
typedef struct {
    tak_bertanda32_t frequency;     /* Frequency in Hz */
    tak_bertanda32_t resolution_ns; /* Resolution in nanoseconds */
    tak_bertanda64_t ticks;         /* Current tick count */
    bool_t supports_one_shot;       /* One-shot mode supported */
    bool_t supports_periodic;       /* Periodic mode supported */
    bool_t supports_64bit;          /* 64-bit counter */
    bool_t reserved;
} hal_timer_info_t;

/* Struktur informasi interupsi */
typedef struct {
    tak_bertanda32_t irq_count;     /* Number of IRQs */
    tak_bertanda32_t vector_base;   /* Vector base */
    bool_t has_pic;                 /* Has PIC */
    bool_t has_apic;                /* Has APIC */
    bool_t has_ioapic;              /* Has I/O APIC */
    bool_t has_msi;                 /* Has MSI support */
} hal_interrupt_info_t;

/* Struktur informasi memori */
typedef struct {
    tak_bertanda64_t total_bytes;     /* Total memory */
    tak_bertanda64_t available_bytes; /* Available memory */
    tak_bertanda64_t used_bytes;      /* Used memory */
    tak_bertanda32_t page_size;       /* Page size */
    tak_bertanda64_t page_count;      /* Total pages */
    tak_bertanda64_t free_pages;      /* Free pages */
    bool_t has_pae;                   /* PAE supported */
    bool_t has_nx;                    /* NX bit supported */
    bool_t has_pse;                   /* PSE supported */
    bool_t reserved;
} hal_memory_info_t;

/* Struktur informasi console */
typedef struct {
    tak_bertanda32_t width;         /* Screen width in chars/pixels */
    tak_bertanda32_t height;        /* Screen height */
    tak_bertanda32_t bpp;           /* Bits per pixel */
    tak_bertanda32_t pitch;         /* Bytes per line */
    uintptr_t framebuffer;          /* Framebuffer address */
    ukuran_t framebuffer_size;      /* Framebuffer size */
    bool_t is_text;                 /* Text mode */
    bool_t is_graphic;              /* Graphic mode */
    bool_t double_buffer;           /* Double buffering */
    bool_t reserved;
} hal_console_info_t;

/* Struktur state HAL */
typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t status;        /* HAL status */
    tak_bertanda32_t version;       /* HAL version */
    tak_bertanda32_t flags;         /* Flags */
    hal_cpu_info_t cpu;             /* CPU info */
    hal_timer_info_t timer;         /* Timer info */
    hal_interrupt_info_t interrupt; /* Interrupt info */
    hal_memory_info_t memory;       /* Memory info */
    hal_console_info_t console;     /* Console info */
    tak_bertanda32_t uptime_sec;    /* Uptime in seconds */
    tak_bertanda64_t uptime_ticks;  /* Uptime in ticks */
} hal_state_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI HAL UTAMA (MAIN HAL FUNCTIONS)
 * ===========================================================================
 */

/*
 * hal_init
 * --------
 * Inisialisasi HAL.
 * Harus dipanggil pertama kali sebelum fungsi HAL lain.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Status error jika gagal
 */
status_t hal_init(void);

/*
 * hal_shutdown
 * ------------
 * Shutdown HAL.
 * Membersihkan semua resource HAL.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t hal_shutdown(void);

/*
 * hal_get_state
 * -------------
 * Dapatkan state HAL.
 *
 * Return: Pointer ke struktur hal_state_t
 */
const hal_state_t *hal_get_state(void);

/*
 * hal_is_initialized
 * ------------------
 * Cek apakah HAL sudah diinisialisasi.
 *
 * Return: BENAR jika sudah, SALAH jika belum
 */
bool_t hal_is_initialized(void);

/*
 * hal_get_version
 * ---------------
 * Dapatkan versi HAL.
 *
 * Return: Versi dalam format 0xMMmmpp (Major, minor, patch)
 */
tak_bertanda32_t hal_get_version(void);

/*
 * hal_get_arch_name
 * -----------------
 * Dapatkan nama arsitektur.
 *
 * Return: String nama arsitektur
 */
const char *hal_get_arch_name(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI CPU HAL (HAL CPU FUNCTIONS)
 * ===========================================================================
 */

/*
 * hal_cpu_init
 * ------------
 * Inisialisasi subsistem CPU.
 *
 * Return: Status operasi
 */
status_t hal_cpu_init(void);

/*
 * hal_cpu_detect
 * --------------
 * Deteksi informasi CPU.
 *
 * Parameter:
 *   info - Pointer ke buffer untuk menyimpan informasi
 *
 * Return: Status operasi
 */
status_t hal_cpu_detect(hal_cpu_info_t *info);

/*
 * hal_cpu_get_info
 * ----------------
 * Dapatkan informasi CPU (cached).
 *
 * Parameter:
 *   info - Pointer ke buffer untuk menyimpan informasi
 *
 * Return: Status operasi
 */
status_t hal_cpu_get_info(hal_cpu_info_t *info);

/*
 * hal_cpu_get_id
 * --------------
 * Dapatkan ID CPU saat ini.
 *
 * Return: ID CPU
 */
tak_bertanda32_t hal_cpu_get_id(void);

/*
 * hal_cpu_get_count
 * -----------------
 * Dapatkan jumlah CPU.
 *
 * Return: Jumlah CPU
 */
tak_bertanda32_t hal_cpu_get_count(void);

/*
 * hal_cpu_halt
 * ------------
 * Hentikan CPU.
 */
void hal_cpu_halt(void) TIDAK_RETURN;

/*
 * hal_cpu_reset
 * -------------
 * Reset CPU.
 *
 * Parameter:
 *   hard - Jika BENAR, lakukan hard reset
 */
void hal_cpu_reset(bool_t hard) TIDAK_RETURN;

/*
 * hal_cpu_enable_interrupts
 * -------------------------
 * Aktifkan interupsi CPU.
 */
void hal_cpu_enable_interrupts(void);

/*
 * hal_cpu_disable_interrupts
 * --------------------------
 * Nonaktifkan interupsi CPU.
 */
void hal_cpu_disable_interrupts(void);

/*
 * hal_cpu_save_interrupts
 * -----------------------
 * Simpan state interupsi dan nonaktifkan.
 *
 * Return: State interupsi sebelumnya
 */
tak_bertanda32_t hal_cpu_save_interrupts(void);

/*
 * hal_cpu_restore_interrupts
 * --------------------------
 * Restore state interupsi.
 *
 * Parameter:
 *   state - State yang disimpan
 */
void hal_cpu_restore_interrupts(tak_bertanda32_t state);

/*
 * hal_cpu_invalidate_tlb
 * ----------------------
 * Invalidate TLB untuk alamat tertentu.
 *
 * Parameter:
 *   addr - Alamat virtual
 */
void hal_cpu_invalidate_tlb(void *addr);

/*
 * hal_cpu_invalidate_tlb_all
 * --------------------------
 * Invalidate seluruh TLB.
 */
void hal_cpu_invalidate_tlb_all(void);

/*
 * hal_cpu_cache_flush
 * -------------------
 * Flush cache CPU.
 */
void hal_cpu_cache_flush(void);

/*
 * hal_cpu_cache_disable
 * ---------------------
 * Nonaktifkan cache CPU.
 */
void hal_cpu_cache_disable(void);

/*
 * hal_cpu_cache_enable
 * --------------------
 * Aktifkan cache CPU.
 */
void hal_cpu_cache_enable(void);

/*
 * hal_cpu_has_feature
 * -------------------
 * Cek apakah CPU mendukung fitur tertentu.
 *
 * Parameter:
 *   feature - Flag fitur (HAL_CPU_FEATURE_*)
 *
 * Return: BENAR jika didukung
 */
bool_t hal_cpu_has_feature(tak_bertanda32_t feature);

/*
 * hal_cpu_get_freq
 * ----------------
 * Dapatkan frekuensi CPU dalam kHz.
 *
 * Return: Frekuensi dalam kHz
 */
tak_bertanda32_t hal_cpu_get_freq(void);

/*
 * hal_cpu_delay_us
 * ----------------
 * Delay dalam microsecond (busy wait).
 *
 * Parameter:
 *   us - Jumlah microsecond
 */
void hal_cpu_delay_us(tak_bertanda32_t us);

/*
 * hal_cpu_delay_ms
 * ----------------
 * Delay dalam millisecond (busy wait).
 *
 * Parameter:
 *   ms - Jumlah millisecond
 */
void hal_cpu_delay_ms(tak_bertanda32_t ms);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI TIMER HAL (HAL TIMER FUNCTIONS)
 * ===========================================================================
 */

/*
 * hal_timer_init
 * --------------
 * Inisialisasi subsistem timer.
 *
 * Return: Status operasi
 */
status_t hal_timer_init(void);

/*
 * hal_timer_get_info
 * ------------------
 * Dapatkan informasi timer.
 *
 * Parameter:
 *   info - Pointer ke buffer
 *
 * Return: Status operasi
 */
status_t hal_timer_get_info(hal_timer_info_t *info);

/*
 * hal_timer_get_ticks
 * -------------------
 * Dapatkan jumlah tick timer.
 *
 * Return: Jumlah tick sejak boot
 */
tak_bertanda64_t hal_timer_get_ticks(void);

/*
 * hal_timer_get_frequency
 * -----------------------
 * Dapatkan frekuensi timer.
 *
 * Return: Frekuensi dalam Hz
 */
tak_bertanda32_t hal_timer_get_frequency(void);

/*
 * hal_timer_get_uptime
 * --------------------
 * Dapatkan uptime sistem.
 *
 * Return: Uptime dalam detik
 */
tak_bertanda64_t hal_timer_get_uptime(void);

/*
 * hal_timer_get_uptime_ms
 * -----------------------
 * Dapatkan uptime dalam millisecond.
 *
 * Return: Uptime dalam millisecond
 */
tak_bertanda64_t hal_timer_get_uptime_ms(void);

/*
 * hal_timer_sleep
 * ---------------
 * Sleep untuk durasi tertentu.
 *
 * Parameter:
 *   milliseconds - Durasi sleep dalam millisecond
 */
void hal_timer_sleep(tak_bertanda32_t milliseconds);

/*
 * hal_timer_sleep_us
 * ------------------
 * Sleep dalam microsecond.
 *
 * Parameter:
 *   microseconds - Durasi sleep
 */
void hal_timer_sleep_us(tak_bertanda32_t microseconds);

/*
 * hal_timer_set_handler
 * ---------------------
 * Set handler untuk timer interrupt.
 *
 * Parameter:
 *   handler - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_timer_set_handler(void (*handler)(void));

/*
 * hal_timer_calibrate
 * -------------------
 * Kalibrasi timer.
 *
 * Return: Status operasi
 */
status_t hal_timer_calibrate(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERUPSI HAL (HAL INTERRUPT FUNCTIONS)
 * ===========================================================================
 */

/*
 * hal_interrupt_init
 * ------------------
 * Inisialisasi subsistem interupsi.
 *
 * Return: Status operasi
 */
status_t hal_interrupt_init(void);

/*
 * hal_interrupt_get_info
 * ----------------------
 * Dapatkan informasi interupsi.
 *
 * Parameter:
 *   info - Pointer ke buffer
 *
 * Return: Status operasi
 */
status_t hal_interrupt_get_info(hal_interrupt_info_t *info);

/*
 * hal_interrupt_enable
 * --------------------
 * Aktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_enable(tak_bertanda32_t irq);

/*
 * hal_interrupt_disable
 * ---------------------
 * Nonaktifkan IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_disable(tak_bertanda32_t irq);

/*
 * hal_interrupt_mask
 * ------------------
 * Mask IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_mask(tak_bertanda32_t irq);

/*
 * hal_interrupt_unmask
 * --------------------
 * Unmask IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Status operasi
 */
status_t hal_interrupt_unmask(tak_bertanda32_t irq);

/*
 * hal_interrupt_acknowledge
 * -------------------------
 * Acknowledge IRQ (End of Interrupt).
 *
 * Parameter:
 *   irq - Nomor IRQ
 */
void hal_interrupt_acknowledge(tak_bertanda32_t irq);

/*
 * hal_interrupt_set_handler
 * -------------------------
 * Set handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq     - Nomor IRQ
 *   handler - Fungsi handler
 *
 * Return: Status operasi
 */
status_t hal_interrupt_set_handler(tak_bertanda32_t irq, void (*handler)(void));

/*
 * hal_interrupt_get_handler
 * -------------------------
 * Dapatkan handler untuk IRQ tertentu.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: Pointer ke handler, atau NULL jika tidak ada
 */
void (*hal_interrupt_get_handler(tak_bertanda32_t irq))(void);

/*
 * hal_interrupt_is_pending
 * ------------------------
 * Cek apakah IRQ sedang pending.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: BENAR jika pending
 */
bool_t hal_interrupt_is_pending(tak_bertanda32_t irq);

/*
 * hal_interrupt_is_masked
 * -----------------------
 * Cek apakah IRQ di-mask.
 *
 * Parameter:
 *   irq - Nomor IRQ
 *
 * Return: BENAR jika di-mask
 */
bool_t hal_interrupt_is_masked(tak_bertanda32_t irq);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI MEMORI HAL (HAL MEMORY FUNCTIONS)
 * ===========================================================================
 */

/*
 * hal_memory_init
 * ---------------
 * Inisialisasi subsistem memori.
 *
 * Parameter:
 *   bootinfo - Pointer ke informasi boot
 *
 * Return: Status operasi
 */
status_t hal_memory_init(void *bootinfo);

/*
 * hal_memory_get_info
 * -------------------
 * Dapatkan informasi memori.
 *
 * Parameter:
 *   info - Pointer ke buffer
 *
 * Return: Status operasi
 */
status_t hal_memory_get_info(hal_memory_info_t *info);

/*
 * hal_memory_get_free_pages
 * -------------------------
 * Dapatkan jumlah halaman bebas.
 *
 * Return: Jumlah halaman bebas
 */
tak_bertanda64_t hal_memory_get_free_pages(void);

/*
 * hal_memory_get_total_pages
 * --------------------------
 * Dapatkan jumlah total halaman.
 *
 * Return: Jumlah total halaman
 */
tak_bertanda64_t hal_memory_get_total_pages(void);

/*
 * hal_memory_get_page_size
 * ------------------------
 * Dapatkan ukuran halaman.
 *
 * Return: Ukuran halaman dalam byte
 */
tak_bertanda32_t hal_memory_get_page_size(void);

/*
 * hal_memory_alloc_page
 * ---------------------
 * Alokasikan satu halaman fisik.
 *
 * Return: Alamat fisik halaman, atau 0 jika gagal
 */
alamat_fisik_t hal_memory_alloc_page(void);

/*
 * hal_memory_free_page
 * --------------------
 * Bebaskan satu halaman fisik.
 *
 * Parameter:
 *   addr - Alamat fisik halaman
 *
 * Return: Status operasi
 */
status_t hal_memory_free_page(alamat_fisik_t addr);

/*
 * hal_memory_alloc_pages
 * ----------------------
 * Alokasikan multiple halaman fisik berurutan.
 *
 * Parameter:
 *   count - Jumlah halaman
 *
 * Return: Alamat fisik halaman pertama, atau 0 jika gagal
 */
alamat_fisik_t hal_memory_alloc_pages(tak_bertanda32_t count);

/*
 * hal_memory_free_pages
 * ---------------------
 * Bebaskan multiple halaman fisik.
 *
 * Parameter:
 *   addr  - Alamat fisik halaman pertama
 *   count - Jumlah halaman
 *
 * Return: Status operasi
 */
status_t hal_memory_free_pages(alamat_fisik_t addr, tak_bertanda32_t count);

/*
 * hal_memory_map_page
 * -------------------
 * Map halaman fisik ke alamat virtual.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *   paddr - Alamat fisik
 *   flags - Flag mapping
 *
 * Return: Status operasi
 */
status_t hal_memory_map_page(alamat_virtual_t vaddr, alamat_fisik_t paddr,
                              tak_bertanda32_t flags);

/*
 * hal_memory_unmap_page
 * ---------------------
 * Unmap halaman dari alamat virtual.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Status operasi
 */
status_t hal_memory_unmap_page(alamat_virtual_t vaddr);

/*
 * hal_memory_get_physical
 * -----------------------
 * Dapatkan alamat fisik dari alamat virtual.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Alamat fisik, atau 0 jika tidak mapped
 */
alamat_fisik_t hal_memory_get_physical(alamat_virtual_t vaddr);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI CONSOLE HAL (HAL CONSOLE FUNCTIONS)
 * ===========================================================================
 */

/*
 * hal_console_init
 * ----------------
 * Inisialisasi console.
 *
 * Return: Status operasi
 */
status_t hal_console_init(void);

/*
 * hal_console_get_info
 * --------------------
 * Dapatkan informasi console.
 *
 * Parameter:
 *   info - Pointer ke buffer
 *
 * Return: Status operasi
 */
status_t hal_console_get_info(hal_console_info_t *info);

/*
 * hal_console_putchar
 * -------------------
 * Tampilkan satu karakter.
 *
 * Parameter:
 *   c - Karakter
 *
 * Return: Status operasi
 */
status_t hal_console_putchar(char c);

/*
 * hal_console_puts
 * ----------------
 * Tampilkan string.
 *
 * Parameter:
 *   str - String
 *
 * Return: Status operasi
 */
status_t hal_console_puts(const char *str);

/*
 * hal_console_write
 * -----------------
 * Tulis buffer ke console.
 *
 * Parameter:
 *   buf - Buffer
 *   len - Panjang buffer
 *
 * Return: Jumlah byte yang ditulis
 */
ukuran_t hal_console_write(const char *buf, ukuran_t len);

/*
 * hal_console_clear
 * -----------------
 * Bersihkan layar.
 *
 * Return: Status operasi
 */
status_t hal_console_clear(void);

/*
 * hal_console_set_color
 * ---------------------
 * Set warna teks.
 *
 * Parameter:
 *   fg - Warna foreground
 *   bg - Warna background
 *
 * Return: Status operasi
 */
status_t hal_console_set_color(tak_bertanda8_t fg, tak_bertanda8_t bg);

/*
 * hal_console_set_cursor
 * ----------------------
 * Set posisi cursor.
 *
 * Parameter:
 *   x - Posisi X (kolom)
 *   y - Posisi Y (baris)
 *
 * Return: Status operasi
 */
status_t hal_console_set_cursor(tak_bertanda32_t x, tak_bertanda32_t y);

/*
 * hal_console_get_cursor
 * ----------------------
 * Dapatkan posisi cursor.
 *
 * Parameter:
 *   x - Pointer untuk menyimpan posisi X
 *   y - Pointer untuk menyimpan posisi Y
 *
 * Return: Status operasi
 */
status_t hal_console_get_cursor(tak_bertanda32_t *x, tak_bertanda32_t *y);

/*
 * hal_console_hide_cursor
 * -----------------------
 * Sembunyikan cursor.
 */
void hal_console_hide_cursor(void);

/*
 * hal_console_show_cursor
 * -----------------------
 * Tampilkan cursor.
 */
void hal_console_show_cursor(void);

/*
 * hal_console_scroll
 * ------------------
 * Scroll layar.
 *
 * Parameter:
 *   lines - Jumlah baris (positif = ke atas)
 *
 * Return: Status operasi
 */
status_t hal_console_scroll(tak_bertanda32_t lines);

/*
 * hal_console_get_size
 * --------------------
 * Dapatkan ukuran layar.
 *
 * Parameter:
 *   width  - Pointer untuk menyimpan lebar
 *   height - Pointer untuk menyimpan tinggi
 *
 * Return: Status operasi
 */
status_t hal_console_get_size(tak_bertanda32_t *width, tak_bertanda32_t *height);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI I/O PORT HAL (HAL I/O PORT FUNCTIONS)
 * ===========================================================================
 */

/*
 * hal_io_read_8
 * -------------
 * Baca byte dari port I/O.
 *
 * Parameter:
 *   port - Nomor port
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda8_t hal_io_read_8(tak_bertanda16_t port);

/*
 * hal_io_write_8
 * --------------
 * Tulis byte ke port I/O.
 *
 * Parameter:
 *   port  - Nomor port
 *   value - Nilai yang ditulis
 */
void hal_io_write_8(tak_bertanda16_t port, tak_bertanda8_t value);

/*
 * hal_io_read_16
 * --------------
 * Baca word dari port I/O.
 *
 * Parameter:
 *   port - Nomor port
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda16_t hal_io_read_16(tak_bertanda16_t port);

/*
 * hal_io_write_16
 * ---------------
 * Tulis word ke port I/O.
 *
 * Parameter:
 *   port  - Nomor port
 *   value - Nilai yang ditulis
 */
void hal_io_write_16(tak_bertanda16_t port, tak_bertanda16_t value);

/*
 * hal_io_read_32
 * --------------
 * Baca dword dari port I/O.
 *
 * Parameter:
 *   port - Nomor port
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda32_t hal_io_read_32(tak_bertanda16_t port);

/*
 * hal_io_write_32
 * ---------------
 * Tulis dword ke port I/O.
 *
 * Parameter:
 *   port  - Nomor port
 *   value - Nilai yang ditulis
 */
void hal_io_write_32(tak_bertanda16_t port, tak_bertanda32_t value);

/*
 * hal_io_delay
 * ------------
 * Delay I/O (kira-kira 1 microsecond).
 */
void hal_io_delay(void);

/*
 * hal_io_read_string_8
 * --------------------
 * Baca string byte dari port I/O.
 *
 * Parameter:
 *   port - Nomor port
 *   buf  - Buffer tujuan
 *   count - Jumlah byte
 */
void hal_io_read_string_8(tak_bertanda16_t port, void *buf, ukuran_t count);

/*
 * hal_io_write_string_8
 * ---------------------
 * Tulis string byte ke port I/O.
 *
 * Parameter:
 *   port - Nomor port
 *   buf  - Buffer sumber
 *   count - Jumlah byte
 */
void hal_io_write_string_8(tak_bertanda16_t port, const void *buf,
                            ukuran_t count);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI MMIO HAL (HAL MMIO FUNCTIONS)
 * ===========================================================================
 */

/*
 * hal_mmio_read_8
 * ---------------
 * Baca byte dari memory-mapped I/O.
 *
 * Parameter:
 *   addr - Alamat MMIO
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda8_t hal_mmio_read_8(const void *addr);

/*
 * hal_mmio_write_8
 * ----------------
 * Tulis byte ke memory-mapped I/O.
 *
 * Parameter:
 *   addr  - Alamat MMIO
 *   value - Nilai yang ditulis
 */
void hal_mmio_write_8(void *addr, tak_bertanda8_t value);

/*
 * hal_mmio_read_16
 * ----------------
 * Baca word dari memory-mapped I/O.
 *
 * Parameter:
 *   addr - Alamat MMIO
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda16_t hal_mmio_read_16(const void *addr);

/*
 * hal_mmio_write_16
 * -----------------
 * Tulis word ke memory-mapped I/O.
 *
 * Parameter:
 *   addr  - Alamat MMIO
 *   value - Nilai yang ditulis
 */
void hal_mmio_write_16(void *addr, tak_bertanda16_t value);

/*
 * hal_mmio_read_32
 * ----------------
 * Baca dword dari memory-mapped I/O.
 *
 * Parameter:
 *   addr - Alamat MMIO
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda32_t hal_mmio_read_32(const void *addr);

/*
 * hal_mmio_write_32
 * -----------------
 * Tulis dword ke memory-mapped I/O.
 *
 * Parameter:
 *   addr  - Alamat MMIO
 *   value - Nilai yang ditulis
 */
void hal_mmio_write_32(void *addr, tak_bertanda32_t value);

/*
 * hal_mmio_read_64
 * ----------------
 * Baca qword dari memory-mapped I/O.
 *
 * Parameter:
 *   addr - Alamat MMIO
 *
 * Return: Nilai yang dibaca
 */
tak_bertanda64_t hal_mmio_read_64(const void *addr);

/*
 * hal_mmio_write_64
 * -----------------
 * Tulis qword ke memory-mapped I/O.
 *
 * Parameter:
 *   addr  - Alamat MMIO
 *   value - Nilai yang ditulis
 */
void hal_mmio_write_64(void *addr, tak_bertanda64_t value);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI MSR (MODEL SPECIFIC REGISTERS)
 * ===========================================================================
 */

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

/*
 * hal_msr_read
 * ------------
 * Baca MSR (Model Specific Register).
 *
 * Parameter:
 *   msr - Nomor MSR
 *
 * Return: Nilai MSR (64-bit)
 */
tak_bertanda64_t hal_msr_read(tak_bertanda32_t msr);

/*
 * hal_msr_write
 * -------------
 * Tulis MSR (Model Specific Register).
 *
 * Parameter:
 *   msr   - Nomor MSR
 *   value - Nilai yang ditulis
 */
void hal_msr_write(tak_bertanda32_t msr, tak_bertanda64_t value);

/*
 * hal_msr_exists
 * --------------
 * Cek apakah MSR ada.
 *
 * Parameter:
 *   msr - Nomor MSR
 *
 * Return: BENAR jika ada
 */
bool_t hal_msr_exists(tak_bertanda32_t msr);

#endif /* ARSITEKTUR_X86 || ARSITEKTUR_X86_64 */

/*
 * ===========================================================================
 * DEKLARASI VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ===========================================================================
 */

/* State HAL global */
extern hal_state_t g_hal_state;

#endif /* INTI_HAL_HAL_H */
