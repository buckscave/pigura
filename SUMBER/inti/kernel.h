/*
 * PIGURA OS - KERNEL.H
 * --------------------
 * Header utama kernel Pigura OS.
 *
 * Berkas ini menyatukan semua header kernel dan mendefinisikan
 * fungsi-fungsi utama yang digunakan di seluruh kernel.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef INTI_KERNEL_H
#define INTI_KERNEL_H

/*
 * ============================================================================
 * INCLUDE HEADER STANDAR (STANDARD HEADER INCLUDES)
 * ============================================================================
 */

/* Header inti kernel */
#include "types.h"
#include "konstanta.h"
#include "makro.h"
#include "config.h"
#include "panic.h"

/*
 * ============================================================================
 * VERSI KERNEL (KERNEL VERSION)
 * ============================================================================
 */

/* Macro untuk mendapatkan versi kernel sebagai string */
#define KERNEL_VERSI_STR() \
    TOSTRING(PIGURA_VERSI_MAJOR) "." \
    TOSTRING(PIGURA_VERSI_MINOR) "." \
    TOSTRING(PIGURA_VERSI_PATCH)

/* Cek versi kernel */
#define KERNEL_VERSI(major, minor, patch) \
    (((major) << 16) | ((minor) << 8) | (patch))

#define KERNEL_VERSI_SEKARANG \
    KERNEL_VERSI(PIGURA_VERSI_MAJOR, PIGURA_VERSI_MINOR, PIGURA_VERSI_PATCH)

/*
 * ============================================================================
 * STRUKTUR DATA GLOBAL (GLOBAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur informasi memori dari bootloader */
typedef struct {
    tak_bertanda64_t total;         /* Total memori dalam byte */
    tak_bertanda64_t tersedia;      /* Memori tersedia */
    tak_bertanda64_t kernel_start;  /* Alamat awal kernel */
    tak_bertanda64_t kernel_end;    /* Alamat akhir kernel */
    tak_bertanda64_t heap_start;    /* Alamat awal heap */
    tak_bertanda64_t heap_end;      /* Alamat akhir heap */
} info_memori_t;

/* Struktur informasi sistem */
typedef struct {
    char nama[32];                  /* Nama sistem */
    char versi[16];                 /* Versi kernel */
    char arsitektur[16];            /* Arsitektur target */
    char hostname[64];              /* Nama host */
    tak_bertanda64_t uptime;        /* Uptime dalam detik */
    tak_bertanda64_t boot_time;     /* Waktu boot (epoch) */
    tak_bertanda32_t cpu_count;     /* Jumlah CPU */
    info_memori_t memori;           /* Informasi memori */
    tak_bertanda32_t proses_count;  /* Jumlah proses */
    tak_bertanda32_t thread_count;  /* Jumlah thread */
} info_sistem_t;

/* Struktur boot info dari multiboot */
typedef struct {
    tak_bertanda32_t flags;         /* Flag multiboot */
    tak_bertanda32_t mem_lower;     /* Memori lower (KB) */
    tak_bertanda32_t mem_upper;     /* Memori upper (KB) */
    tak_bertanda32_t boot_device;   /* Boot device */
    tak_bertanda32_t cmdline;       /* Command line */
    tak_bertanda32_t mods_count;    /* Jumlah module */
    tak_bertanda32_t mods_addr;     /* Alamat module */
    tak_bertanda32_t syms[4];       /* Symbol table info */
    tak_bertanda32_t mmap_length;   /* Panjang memory map */
    tak_bertanda32_t mmap_addr;     /* Alamat memory map */
    tak_bertanda32_t drives_length; /* Panjang drive info */
    tak_bertanda32_t drives_addr;   /* Alamat drive info */
    tak_bertanda32_t config_table;  /* Konfigurasi ROM */
    tak_bertanda32_t boot_loader_name; /* Nama bootloader */
    tak_bertanda32_t apm_table;     /* APM table */
    tak_bertanda32_t vbe_control_info; /* VBE info */
    tak_bertanda32_t vbe_mode_info;    /* VBE mode info */
    tak_bertanda16_t vbe_mode;         /* VBE mode */
    tak_bertanda16_t vbe_interface_seg; /* VBE interface */
    tak_bertanda16_t vbe_interface_off;
    tak_bertanda16_t vbe_interface_len;
} multiboot_info_t;

/* Entry memory map */
typedef struct {
    tak_bertanda32_t size;          /* Ukuran entry ini */
    tak_bertanda64_t base_addr;     /* Alamat awal */
    tak_bertanda64_t length;        /* Panjang region */
    tak_bertanda32_t type;          /* Tipe memori */
} mmap_entry_t;

/* Struktur module multiboot */
typedef struct {
    tak_bertanda32_t mod_start;     /* Alamat awal module */
    tak_bertanda32_t mod_end;       /* Alamat akhir module */
    tak_bertanda32_t string;        /* String module */
    tak_bertanda32_t reserved;      /* Reserved */
} module_t;

/*
 * ============================================================================
 * VARIABEL GLOBAL KERNEL (KERNEL GLOBAL VARIABLES)
 * ============================================================================
 */

/* Informasi sistem global */
extern info_sistem_t g_info_sistem;

/* Boot info dari multiboot */
extern multiboot_info_t *g_boot_info;

/* Flag kernel sudah diinisialisasi */
extern volatile int g_kernel_diinisialisasi;

/* Uptime counter (dalam ticks) */
extern volatile tak_bertanda64_t g_uptime_ticks;

/* Waktu boot */
extern tak_bertanda64_t g_boot_time;

/* Pointer ke proses saat ini */
extern struct pcb *g_proses_sekarang;

/* Jumlah CPU aktif */
extern tak_bertanda32_t g_cpu_count;

/* CPU ID saat ini */
extern tak_bertanda32_t g_cpu_id;

/*
 * ============================================================================
 * DEKLARASI FUNGSI KERNEL UTAMA (MAIN KERNEL FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_main
 * -----------
 * Entry point utama kernel.
 * Fungsi ini dipanggil dari kode bootstrap.
 *
 * Parameter:
 *   magic    - Magic number multiboot
 *   bootinfo - Pointer ke struktur multiboot info
 *
 * Return: Tidak pernah return
 */
void kernel_main(tak_bertanda32_t magic,
                 multiboot_info_t *bootinfo) __attribute__((noreturn));

/*
 * kernel_init
 * -----------
 * Inisialisasi kernel.
 * Dipanggil dari kernel_main setelah boot awal.
 *
 * Parameter:
 *   bootinfo - Pointer ke struktur multiboot info
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 *   Status error jika gagal
 */
status_t kernel_init(multiboot_info_t *bootinfo);

/*
 * kernel_start
 * ------------
 * Mulai operasi normal kernel.
 * Memulai scheduler dan beralih ke user mode.
 *
 * Return: Tidak pernah return
 */
void kernel_start(void) __attribute__((noreturn));

/*
 * kernel_shutdown
 * ---------------
 * Shutdown kernel dengan aman.
 *
 * Parameter:
 *   reboot - Jika tidak nol, lakukan reboot
 *
 * Return: Tidak pernah return
 */
void kernel_shutdown(int reboot) __attribute__((noreturn));

/*
 * ============================================================================
 * DEKLARASI FUNGSI OUTPUT (OUTPUT FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_printf
 * -------------
 * Print formatted string ke console kernel.
 *
 * Parameter:
 *   format - Format string
 *   ...    - Argumen format
 *
 * Return: Jumlah karakter yang dicetak
 */
int kernel_printf(const char *format, ...);

/*
 * kernel_printk
 * -------------
 * Print ke kernel log dengan level.
 *
 * Parameter:
 *   level  - Level log (0=error, 1=warn, 2=info, 3=debug)
 *   format - Format string
 *   ...    - Argumen format
 *
 * Return: Jumlah karakter yang dicetak
 */
int kernel_printk(int level, const char *format, ...);

/*
 * kernel_puts
 * -----------
 * Print string ke console kernel.
 *
 * Parameter:
 *   str - String yang akan dicetak
 *
 * Return: 0 jika berhasil
 */
int kernel_puts(const char *str);

/*
 * kernel_putchar
 * --------------
 * Print satu karakter ke console kernel.
 *
 * Parameter:
 *   c - Karakter yang akan dicetak
 *
 * Return: Karakter yang dicetak, atau EOF jika error
 */
int kernel_putchar(int c);

/*
 * kernel_clear_screen
 * -------------------
 * Bersihkan layar console.
 */
void kernel_clear_screen(void);

/*
 * kernel_set_color
 * ----------------
 * Set warna teks console.
 *
 * Parameter:
 *   fg - Warna foreground
 *   bg - Warna background
 */
void kernel_set_color(tak_bertanda8_t fg, tak_bertanda8_t bg);

/*
 * ============================================================================
 * DEKLARASI FUNGSI LOGGING (LOGGING FUNCTIONS)
 * ============================================================================
 */

/* Level log */
#define LOG_ERROR   1
#define LOG_WARN    2
#define LOG_INFO    3
#define LOG_DEBUG   4

/*
 * log_error
 * ---------
 * Log pesan error.
 */
#define log_error(fmt, ...) \
    kernel_printk(LOG_ERROR, "[ERROR] " fmt, ##__VA_ARGS__)

/*
 * log_warn
 * --------
 * Log pesan warning.
 */
#define log_warn(fmt, ...) \
    kernel_printk(LOG_WARN, "[WARN] " fmt, ##__VA_ARGS__)

/*
 * log_info
 * --------
 * Log pesan info.
 */
#define log_info(fmt, ...) \
    kernel_printk(LOG_INFO, "[INFO] " fmt, ##__VA_ARGS__)

/*
 * log_debug
 * ---------
 * Log pesan debug.
 */
#ifdef DEBUG
    #define log_debug(fmt, ...) \
        kernel_printk(LOG_DEBUG, "[DEBUG] " fmt, ##__VA_ARGS__)
#else
    #define log_debug(fmt, ...) ((void)0)
#endif

/*
 * ============================================================================
 * DEKLARASI FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_delay
 * ------------
 * Delay sederhana (busy wait).
 *
 * Parameter:
 *   ms - Jumlah milidetik delay
 */
void kernel_delay(tak_bertanda32_t ms);

/*
 * kernel_sleep
 * ------------
 * Sleep kernel untuk durasi tertentu.
 *
 * Parameter:
 *   ms - Jumlah milidetik sleep
 */
void kernel_sleep(tak_bertanda32_t ms);

/*
 * kernel_get_uptime
 * -----------------
 * Dapatkan uptime sistem.
 *
 * Return: Uptime dalam detik
 */
tak_bertanda64_t kernel_get_uptime(void);

/*
 * kernel_get_ticks
 * ----------------
 * Dapatkan jumlah timer ticks.
 *
 * Return: Jumlah ticks sejak boot
 */
tak_bertanda64_t kernel_get_ticks(void);

/*
 * kernel_get_info
 * ---------------
 * Dapatkan informasi sistem.
 *
 * Return: Pointer ke struktur info_sistem_t
 */
const info_sistem_t *kernel_get_info(void);

/*
 * kernel_get_arch
 * ---------------
 * Dapatkan nama arsitektur.
 *
 * Return: String nama arsitektur
 */
const char *kernel_get_arch(void);

/*
 * kernel_get_version
 * ------------------
 * Dapatkan versi kernel.
 *
 * Return: String versi kernel
 */
const char *kernel_get_version(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI MEMORY (MEMORY FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_memcpy
 * -------------
 * Copy memory block.
 *
 * Parameter:
 *   dest - Destination buffer
 *   src  - Source buffer
 *   n    - Jumlah byte
 *
 * Return: Pointer ke dest
 */
void *kernel_memcpy(void *dest, const void *src, ukuran_t n);

/*
 * kernel_memset
 * -------------
 * Set memory block dengan nilai.
 *
 * Parameter:
 *   s   - Buffer
 *   c   - Nilai byte
 *   n   - Jumlah byte
 *
 * Return: Pointer ke s
 */
void *kernel_memset(void *s, int c, ukuran_t n);

/*
 * kernel_memmove
 * --------------
 * Move memory block (handles overlap).
 *
 * Parameter:
 *   dest - Destination buffer
 *   src  - Source buffer
 *   n    - Jumlah byte
 *
 * Return: Pointer ke dest
 */
void *kernel_memmove(void *dest, const void *src, ukuran_t n);

/*
 * kernel_memcmp
 * -------------
 * Compare memory blocks.
 *
 * Parameter:
 *   s1 - Block pertama
 *   s2 - Block kedua
 *   n  - Jumlah byte
 *
 * Return: <0, 0, >0 jika s1 < s2, s1 == s2, s1 > s2
 */
int kernel_memcmp(const void *s1, const void *s2, ukuran_t n);

/*
 * ============================================================================
 * DEKLARASI FUNGSI STRING (STRING FUNCTIONS)
 * ============================================================================
 */

/*
 * kernel_strlen
 * -------------
 * Hitung panjang string.
 *
 * Parameter:
 *   s - String
 *
 * Return: Panjang string
 */
ukuran_t kernel_strlen(const char *s);

/*
 * kernel_strncpy
 * --------------
 * Copy string dengan batas.
 *
 * Parameter:
 *   dest - Buffer destination
 *   src  - String source
 *   n    - Ukuran buffer
 *
 * Return: Pointer ke dest
 */
char *kernel_strncpy(char *dest, const char *src, ukuran_t n);

/*
 * kernel_strcmp
 * -------------
 * Compare dua string.
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *
 * Return: <0, 0, >0 jika s1 < s2, s1 == s2, s1 > s2
 */
int kernel_strcmp(const char *s1, const char *s2);

/*
 * kernel_strncmp
 * --------------
 * Compare dua string dengan batas.
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *   n  - Jumlah karakter maksimum
 *
 * Return: <0, 0, >0 jika s1 < s2, s1 == s2, s1 > s2
 */
int kernel_strncmp(const char *s1, const char *s2, ukuran_t n);

/*
 * ============================================================================
 * INLINE ASSEMBLY HELPERS (INLINE ASSEMBLY HELPERS)
 * ============================================================================
 */

/* Hentikan CPU */
static inline void cpu_halt(void)
{
    __asm__ __volatile__("hlt");
}

/* Disable interrupt */
static inline void cpu_disable_irq(void)
{
    __asm__ __volatile__("cli");
}

/* Enable interrupt */
static inline void cpu_enable_irq(void)
{
    __asm__ __volatile__("sti");
}

/* Tidak melakukan apa-apa */
static inline void cpu_nop(void)
{
    __asm__ __volatile__("nop");
}

/* Invalidate TLB untuk satu halaman */
static inline void cpu_invlpg(void *addr)
{
    __asm__ __volatile__("invlpg (%0)" : : "r"(addr) : "memory");
}

/* Reload CR3 */
static inline void cpu_reload_cr3(void)
{
    tak_bertanda32_t cr3;
    __asm__ __volatile__("mov %%cr3, %0\n\t"
                         "mov %0, %%cr3"
                         : "=r"(cr3)
                         :
                         : "memory");
}

/* Baca CR2 */
static inline tak_bertanda32_t cpu_read_cr2(void)
{
    tak_bertanda32_t val;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(val));
    return val;
}

/* Baca CR3 */
static inline tak_bertanda32_t cpu_read_cr3(void)
{
    tak_bertanda32_t val;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(val));
    return val;
}

/* Tulis CR3 */
static inline void cpu_write_cr3(tak_bertanda32_t val)
{
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(val) : "memory");
}

/* Baca CR0 */
static inline tak_bertanda32_t cpu_read_cr0(void)
{
    tak_bertanda32_t val;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(val));
    return val;
}

/* Tulis CR0 */
static inline void cpu_write_cr0(tak_bertanda32_t val)
{
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(val) : "memory");
}

/* Baca byte dari port I/O */
static inline tak_bertanda8_t inb(tak_bertanda16_t port)
{
    tak_bertanda8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* Tulis byte ke port I/O */
static inline void outb(tak_bertanda16_t port, tak_bertanda8_t val)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Baca word dari port I/O */
static inline tak_bertanda16_t inw(tak_bertanda16_t port)
{
    tak_bertanda16_t val;
    __asm__ __volatile__("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* Tulis word ke port I/O */
static inline void outw(tak_bertanda16_t port, tak_bertanda16_t val)
{
    __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* Baca dword dari port I/O */
static inline tak_bertanda32_t inl(tak_bertanda16_t port)
{
    tak_bertanda32_t val;
    __asm__ __volatile__("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* Tulis dword ke port I/O */
static inline void outl(tak_bertanda16_t port, tak_bertanda32_t val)
{
    __asm__ __volatile__("outl %0, %1" : : "a"(val), "Nd"(port));
}

/* IO delay */
static inline void io_delay(void)
{
    outb(0x80, 0);
}

/* Simpan flags dan disable interrupt */
static inline tak_bertanda32_t cpu_save_flags(void)
{
    tak_bertanda32_t flags;
    __asm__ __volatile__("pushfl\n\t"
                         "popl %0\n\t"
                         "cli"
                         : "=rm"(flags)
                         :
                         : "memory");
    return flags;
}

/* Restore flags */
static inline void cpu_restore_flags(tak_bertanda32_t flags)
{
    __asm__ __volatile__("pushl %0\n\t"
                         "popfl"
                         :
                         : "g"(flags)
                         : "memory", "cc");
}

/*
 * ============================================================================
 * MAKRO HELPER UNTUK INTERRUPT (INTERRUPT HELPER MACROS)
 * ============================================================================
 */

/* Disable interrupt dan simpan state */
#define IRQ_DISABLE_SAVE(flags) \
    do { (flags) = cpu_save_flags(); } while (0)

/* Restore interrupt state */
#define IRQ_RESTORE(flags) \
    do { cpu_restore_flags(flags); } while (0)

/* Critical section */
#define CRITICAL_SECTION(block) \
    do { \
        tak_bertanda32_t _flags; \
        IRQ_DISABLE_SAVE(_flags); \
        block; \
        IRQ_RESTORE(_flags); \
    } while (0)

#endif /* INTI_KERNEL_H */
