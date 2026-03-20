/*
 * PIGURA OS - HAL.H
 * -----------------
 * Header Hardware Abstraction Layer (HAL).
 *
 * HAL menyediakan interface yang portabel untuk operasi hardware,
 * memungkinkan kernel berjalan di berbagai arsitektur tanpa modifikasi.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef INTI_HAL_HAL_H
#define INTI_HAL_HAL_H

#include "../types.h"
#include "../konstanta.h"

/*
 * ============================================================================
 * STRUKTUR DATA HAL (HAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur informasi CPU */
typedef struct {
    tak_bertanda32_t id;            /* CPU ID */
    tak_bertanda32_t apic_id;       /* APIC ID */
    char vendor[13];                /* Vendor string */
    char brand[49];                 /* Brand string */
    tak_bertanda32_t family;        /* Family */
    tak_bertanda32_t model;         /* Model */
    tak_bertanda32_t stepping;      /* Stepping */
    tak_bertanda32_t features;      /* Feature flags */
    tak_bertanda32_t cache_line;    /* Cache line size */
    tak_bertanda32_t cache_l1;      /* L1 cache size */
    tak_bertanda32_t cache_l2;      /* L2 cache size */
    tak_bertanda32_t cache_l3;      /* L3 cache size */
    tak_bertanda32_t freq_mhz;      /* Frekuensi dalam MHz */
    tak_bertanda8_t cores;          /* Jumlah core */
    tak_bertanda8_t threads;        /* Jumlah thread */
} hal_cpu_info_t;

/* Struktur informasi timer */
typedef struct {
    tak_bertanda32_t frequency;     /* Frekuensi dalam Hz */
    tak_bertanda32_t resolution_ns; /* Resolusi dalam nanodetik */
    bool_t supports_one_shot;       /* Mendukung mode one-shot */
    bool_t supports_periodic;       /* Mendukung mode periodic */
} hal_timer_info_t;

/* Struktur informasi interupsi */
typedef struct {
    tak_bertanda32_t irq_count;     /* Jumlah IRQ */
    tak_bertanda32_t vector_base;   /* Vektor base */
    bool_t has_apic;                /* Memiliki APIC */
    bool_t has_ioapic;              /* Memiliki I/O APIC */
} hal_interrupt_info_t;

/* Struktur informasi memori */
typedef struct {
    tak_bertanda64_t total_bytes;   /* Total memori */
    tak_bertanda64_t available_bytes; /* Memori tersedia */
    tak_bertanda32_t page_size;     /* Ukuran halaman */
    tak_bertanda32_t page_count;    /* Jumlah halaman */
    bool_t has_pae;                 /* Mendukung PAE */
    bool_t has_nx;                  /* Mendukung NX bit */
} hal_memory_info_t;

/* Struktur state HAL */
typedef struct {
    bool_t initialized;             /* HAL sudah diinisialisasi */
    hal_cpu_info_t cpu;             /* Informasi CPU */
    hal_timer_info_t timer;         /* Informasi timer */
    hal_interrupt_info_t interrupt; /* Informasi interupsi */
    hal_memory_info_t memory;       /* Informasi memori */
} hal_state_t;

/*
 * ============================================================================
 * VARIABEL GLOBAL HAL (HAL GLOBAL VARIABLES)
 * ============================================================================
 */

/* State HAL global */
extern hal_state_t g_hal_state;

/*
 * ============================================================================
 * DEKLARASI FUNGSI HAL UTAMA (MAIN HAL FUNCTIONS)
 * ============================================================================
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
 * ============================================================================
 * DEKLARASI FUNGSI CPU HAL (HAL CPU FUNCTIONS)
 * ============================================================================
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
 * hal_cpu_halt
 * ------------
 * Hentikan CPU.
 * CPU akan berhenti mengeksekusi instruksi sampai interupsi.
 */
void hal_cpu_halt(void) __attribute__((noreturn));

/*
 * hal_cpu_reset
 * -------------
 * Reset CPU.
 *
 * Parameter:
 *   hard - Jika BENAR, lakukan hard reset
 *
 * Return: Tidak pernah return jika berhasil
 */
void hal_cpu_reset(bool_t hard) __attribute__((noreturn));

/*
 * hal_cpu_get_id
 * --------------
 * Dapatkan ID CPU saat ini.
 *
 * Return: ID CPU
 */
tak_bertanda32_t hal_cpu_get_id(void);

/*
 * hal_cpu_get_info
 * ----------------
 * Dapatkan informasi CPU.
 *
 * Parameter:
 *   info - Pointer ke buffer untuk menyimpan informasi
 *
 * Return: Status operasi
 */
status_t hal_cpu_get_info(hal_cpu_info_t *info);

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
 * ============================================================================
 * DEKLARASI FUNGSI TIMER HAL (HAL TIMER FUNCTIONS)
 * ============================================================================
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
 * hal_timer_sleep
 * ---------------
 * Sleep untuk durasi tertentu.
 *
 * Parameter:
 *   milliseconds - Durasi sleep dalam milidetik
 */
void hal_timer_sleep(tak_bertanda32_t milliseconds);

/*
 * hal_timer_delay
 * ---------------
 * Busy-wait delay.
 *
 * Parameter:
 *   microseconds - Durasi delay dalam mikrodetik
 */
void hal_timer_delay(tak_bertanda32_t microseconds);

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
 * ============================================================================
 * DEKLARASI FUNGSI INTERUPSI HAL (HAL INTERRUPT FUNCTIONS)
 * ============================================================================
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
status_t hal_interrupt_set_handler(tak_bertanda32_t irq,
                                   void (*handler)(void));

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
 * ============================================================================
 * DEKLARASI FUNGSI MEMORI HAL (HAL MEMORY FUNCTIONS)
 * ============================================================================
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
 *   info - Pointer ke buffer untuk menyimpan informasi
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
status_t hal_memory_free_pages(alamat_fisik_t addr,
                               tak_bertanda32_t count);

/*
 * ============================================================================
 * DEKLARASI FUNGSI CONSOLE HAL (HAL CONSOLE FUNCTIONS)
 * ============================================================================
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
 * hal_console_scroll
 * ------------------
 * Scroll layar.
 *
 * Parameter:
 *   lines - Jumlah baris (positif = ke atas, negatif = ke bawah)
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
 * ============================================================================
 * DEKLARASI FUNGSI I/O PORT HAL (HAL I/O PORT FUNCTIONS)
 * ============================================================================
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
 *   port - Nomor port
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
 *   port - Nomor port
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
 *   port - Nomor port
 *   value - Nilai yang ditulis
 */
void hal_io_write_32(tak_bertanda16_t port, tak_bertanda32_t value);

/*
 * hal_io_delay
 * ------------
 * Delay I/O (approx 1 microsecond).
 */
void hal_io_delay(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI MMIO HAL (HAL MMIO FUNCTIONS)
 * ============================================================================
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
tak_bertanda8_t hal_mmio_read_8(void *addr);

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
tak_bertanda16_t hal_mmio_read_16(void *addr);

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
tak_bertanda32_t hal_mmio_read_32(void *addr);

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
tak_bertanda64_t hal_mmio_read_64(void *addr);

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

#endif /* INTI_HAL_HAL_H */
