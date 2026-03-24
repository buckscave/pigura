/*
 * PIGURA OS - KERNEL.H
 * =====================
 * Header utama kernel Pigura OS.
 *
 * Berkas ini menyatukan semua header kernel dan mendefinisikan
 * fungsi-fungsi utama yang digunakan di seluruh kernel.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi dideklarasikan secara eksplisit
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#ifndef INTI_KERNEL_H
#define INTI_KERNEL_H

/*
 * ===========================================================================
 * INCLUDE HEADER INTI (CORE HEADERS)
 * ===========================================================================
 * Include urutan penting: types -> config -> konstanta -> makro -> panic
 */

#include "types.h"
#include "config.h"
#include "konstanta.h"
#include "makro.h"
#include "panic.h"
#include "stdarg.h"

/* Header subsistem */
#include "hal/hal.h"
#include "interupsi/isr.h"

/*
 * ===========================================================================
 * FORWARD DECLARATIONS
 * ===========================================================================
 */

struct proses;
struct thread;
struct vm_descriptor;
struct page_directory;
struct file;
struct inode;
struct dentry;
struct superblock;
struct mount;

/*
 * ===========================================================================
 * STRUKTUR DATA GLOBAL (GLOBAL DATA STRUCTURES)
 * ===========================================================================
 */

/* Struktur informasi memori dari bootloader */
typedef struct {
    tak_bertanda64_t total;         /* Total memori dalam byte */
    tak_bertanda64_t tersedia;      /* Memori tersedia */
    tak_bertanda64_t kernel_start;  /* Alamat awal kernel */
    tak_bertanda64_t kernel_end;    /* Alamat akhir kernel */
    tak_bertanda64_t heap_start;    /* Alamat awal heap */
    tak_bertanda64_t heap_end;      /* Alamat akhir heap */
    tak_bertanda64_t stack_start;   /* Alamat awal stack */
    tak_bertanda64_t stack_end;     /* Alamat akhir stack */
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
    tak_bertanda64_t jiffies;       /* Tick count */
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

/* Process type forward declaration */
typedef struct proses proses_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL KERNEL (KERNEL GLOBAL VARIABLES)
 * ===========================================================================
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
extern proses_t *g_proses_sekarang;

/* Jumlah CPU aktif */
extern tak_bertanda32_t g_cpu_count;

/* CPU ID saat ini */
extern tak_bertanda32_t g_cpu_id;

/* Kernel command line */
extern char g_cmdline[256];

/* Bootloader name */
extern char g_bootloader_name[64];

/*
 * ===========================================================================
 * DEKLARASI FUNGSI KERNEL UTAMA (MAIN KERNEL FUNCTIONS)
 * ===========================================================================
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
 */
void kernel_main(tak_bertanda32_t magic, multiboot_info_t *bootinfo)
    TIDAK_RETURN;

/*
 * kernel_init
 * -----------
 * Inisialisasi kernel.
 *
 * Parameter:
 *   bootinfo - Pointer ke struktur multiboot info
 *
 * Return: Status operasi
 */
status_t kernel_init(multiboot_info_t *bootinfo);

/*
 * kernel_start
 * ------------
 * Mulai operasi normal kernel.
 */
void kernel_start(void) TIDAK_RETURN;

/*
 * kernel_shutdown
 * ---------------
 * Shutdown kernel dengan aman.
 *
 * Parameter:
 *   reboot - 0 = power off, 1 = reboot
 */
void kernel_shutdown(int reboot) TIDAK_RETURN;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI OUTPUT (OUTPUT FUNCTIONS)
 * ===========================================================================
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
 *   level  - Level log (LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG)
 *   format - Format string
 *   ...    - Argumen format
 *
 * Return: Jumlah karakter yang dicetak
 */
int kernel_printk(int level, const char *format, ...);

/*
 * kernel_vprintf
 * --------------
 * Print formatted string dengan va_list.
 *
 * Parameter:
 *   format - Format string
 *   args   - va_list argumen
 *
 * Return: Jumlah karakter yang dicetak
 */
int kernel_vprintf(const char *format, va_list args);

/*
 * kernel_vprintk
 * --------------
 * Print ke kernel log dengan va_list.
 *
 * Parameter:
 *   level  - Level log
 *   format - Format string
 *   args   - va_list argumen
 *
 * Return: Jumlah karakter yang dicetak
 */
int kernel_vprintk(int level, const char *format, va_list args);

/*
 * kernel_puts
 * -----------
 * Print string ke console kernel.
 *
 * Parameter:
 *   str - String yang akan dicetak
 *
 * Return: Jumlah karakter yang dicetak
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
 * kernel_snprintf
 * ---------------
 * Format string ke buffer dengan batas ukuran.
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   size   - Ukuran buffer
 *   format - Format string
 *   ...    - Argumen format
 *
 * Return: Jumlah karakter yang akan ditulis (tanpa null terminator)
 */
int kernel_snprintf(char *str, ukuran_t size, const char *format, ...);

/*
 * kernel_vsnprintf
 * ----------------
 * Format string ke buffer dengan va_list.
 *
 * Parameter:
 *   str    - Buffer tujuan
 *   size   - Ukuran buffer
 *   format - Format string
 *   args   - va_list argumen
 *
 * Return: Jumlah karakter yang akan ditulis
 */
int kernel_vsnprintf(char *str, ukuran_t size, const char *format, va_list args);

/*
 * ===========================================================================
 * LOGGING LEVELS
 * ===========================================================================
 */

#define LOG_ERROR 1
#define LOG_WARN  2
#define LOG_INFO  3
#define LOG_DEBUG 4

/* Makro log */
#define log_error(fmt, ...) \
    kernel_printk(LOG_ERROR, "[ERROR] " fmt, ##__VA_ARGS__)

#define log_warn(fmt, ...) \
    kernel_printk(LOG_WARN, "[WARN] " fmt, ##__VA_ARGS__)

#define log_info(fmt, ...) \
    kernel_printk(LOG_INFO, "[INFO] " fmt, ##__VA_ARGS__)

#ifdef DEBUG
#define log_debug(fmt, ...) \
    kernel_printk(LOG_DEBUG, "[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...) ((void)0)
#endif

/*
 * ===========================================================================
 * DEKLARASI FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ===========================================================================
 */

/*
 * kernel_delay
 * ------------
 * Delay sederhana (busy wait).
 *
 * Parameter:
 *   ms - Durasi dalam millisecond
 */
void kernel_delay(tak_bertanda32_t ms);

/*
 * kernel_sleep
 * ------------
 * Sleep kernel untuk durasi tertentu.
 *
 * Parameter:
 *   ms - Durasi dalam millisecond
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
 * kernel_get_hostname
 * -------------------
 * Dapatkan hostname sistem.
 *
 * Return: String hostname
 */
const char *kernel_get_hostname(void);

/*
 * kernel_set_hostname
 * -------------------
 * Set hostname sistem.
 *
 * Parameter:
 *   hostname - Nama host baru
 *
 * Return: Status operasi
 */
status_t kernel_set_hostname(const char *hostname);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI MEMORY (MEMORY FUNCTIONS)
 * ===========================================================================
 */

/*
 * kernel_memcpy
 * -------------
 * Copy memory block.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - Buffer sumber
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
 *   s - Buffer
 *   c - Nilai byte
 *   n - Jumlah byte
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
 *   dest - Buffer tujuan
 *   src  - Buffer sumber
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
 *   s1 - Buffer pertama
 *   s2 - Buffer kedua
 *   n  - Jumlah byte
 *
 * Return: 0 jika sama, <0 jika s1<s2, >0 jika s1>s2
 */
int kernel_memcmp(const void *s1, const void *s2, ukuran_t n);

/*
 * kernel_memchr
 * -------------
 * Cari karakter dalam memory.
 *
 * Parameter:
 *   s - Buffer
 *   c - Karakter yang dicari
 *   n - Jumlah byte
 *
 * Return: Pointer ke karakter, atau NULL jika tidak ditemukan
 */
void *kernel_memchr(const void *s, int c, ukuran_t n);

/*
 * kernel_memset32
 * ---------------
 * Set memory dengan 32-bit value.
 *
 * Parameter:
 *   s   - Buffer
 *   val - Nilai 32-bit
 *   n   - Jumlah dword
 *
 * Return: Pointer ke s
 */
void *kernel_memset32(void *s, tak_bertanda32_t val, ukuran_t n);

/*
 * kernel_memset64
 * ---------------
 * Set memory dengan 64-bit value.
 *
 * Parameter:
 *   s   - Buffer
 *   val - Nilai 64-bit
 *   n   - Jumlah qword
 *
 * Return: Pointer ke s
 */
void *kernel_memset64(void *s, tak_bertanda64_t val, ukuran_t n);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI STRING (STRING FUNCTIONS)
 * ===========================================================================
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
 * kernel_strnlen
 * --------------
 * Hitung panjang string dengan batas.
 *
 * Parameter:
 *   s      - String
 *   maxlen - Panjang maksimum
 *
 * Return: Panjang string
 */
ukuran_t kernel_strnlen(const char *s, ukuran_t maxlen);

/*
 * kernel_strcpy
 * -------------
 * Copy string.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *
 * Return: Pointer ke dest
 */
char *kernel_strcpy(char *dest, const char *src);

/*
 * kernel_strncpy
 * --------------
 * Copy string dengan batas.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *   n    - Ukuran buffer
 *
 * Return: Pointer ke dest
 */
char *kernel_strncpy(char *dest, const char *src, ukuran_t n);

/*
 * kernel_strcat
 * -------------
 * Concatenate string.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *
 * Return: Pointer ke dest
 */
char *kernel_strcat(char *dest, const char *src);

/*
 * kernel_strncat
 * --------------
 * Concatenate string dengan batas.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String sumber
 *   n    - Ukuran buffer tersisa
 *
 * Return: Pointer ke dest
 */
char *kernel_strncat(char *dest, const char *src, ukuran_t n);

/*
 * kernel_strcmp
 * -------------
 * Compare dua string.
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *
 * Return: 0 jika sama, <0 jika s1<s2, >0 jika s1>s2
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
 * Return: 0 jika sama, <0 jika s1<s2, >0 jika s1>s2
 */
int kernel_strncmp(const char *s1, const char *s2, ukuran_t n);

/*
 * kernel_strcasecmp
 * -----------------
 * Compare dua string (case insensitive).
 *
 * Parameter:
 *   s1 - String pertama
 *   s2 - String kedua
 *
 * Return: 0 jika sama
 */
int kernel_strcasecmp(const char *s1, const char *s2);

/*
 * kernel_strchr
 * -------------
 * Cari karakter dalam string.
 *
 * Parameter:
 *   s - String
 *   c - Karakter
 *
 * Return: Pointer ke karakter, atau NULL
 */
char *kernel_strchr(const char *s, int c);

/*
 * kernel_strrchr
 * --------------
 * Cari karakter terakhir dalam string.
 *
 * Parameter:
 *   s - String
 *   c - Karakter
 *
 * Return: Pointer ke karakter, atau NULL
 */
char *kernel_strrchr(const char *s, int c);

/*
 * kernel_strstr
 * -------------
 * Cari substring dalam string.
 *
 * Parameter:
 *   haystack - String sumber
 *   needle   - Substring yang dicari
 *
 * Return: Pointer ke substring, atau NULL
 */
char *kernel_strstr(const char *haystack, const char *needle);

/*
 * kernel_strdup
 * -------------
 * Duplikasi string (harus di-free).
 *
 * Parameter:
 *   s - String
 *
 * Return: Pointer ke string baru, atau NULL
 */
char *kernel_strdup(const char *s);

/*
 * kernel_strndup
 * --------------
 * Duplikasi string dengan batas.
 *
 * Parameter:
 *   s - String
 *   n - Panjang maksimum
 *
 * Return: Pointer ke string baru, atau NULL
 */
char *kernel_strndup(const char *s, ukuran_t n);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI PMM (PHYSICAL MEMORY MANAGER)
 * ===========================================================================
 */

/*
 * pmm_init
 * --------
 * Inisialisasi physical memory manager.
 *
 * Parameter:
 *   mem_size    - Ukuran memori total
 *   bitmap_addr - Alamat bitmap
 *
 * Return: Status operasi
 */
status_t pmm_init(tak_bertanda64_t mem_size, void *bitmap_addr);

/*
 * pmm_add_region
 * --------------
 * Tambah region memori.
 *
 * Parameter:
 *   mulai - Alamat awal
 *   akhir - Alamat akhir
 *   tipe  - Tipe memori
 *
 * Return: Status operasi
 */
status_t pmm_add_region(alamat_fisik_t mulai, alamat_fisik_t akhir,
                        tak_bertanda32_t tipe);

/*
 * pmm_reserve
 * -----------
 * Reserve region memori.
 *
 * Parameter:
 *   mulai  - Alamat awal
 *   ukuran - Ukuran region
 *
 * Return: Status operasi
 */
status_t pmm_reserve(alamat_fisik_t mulai, tak_bertanda64_t ukuran);

/*
 * pmm_alloc_page
 * --------------
 * Alokasikan satu halaman fisik.
 *
 * Return: Alamat fisik halaman, atau 0 jika gagal
 */
alamat_fisik_t pmm_alloc_page(void);

/*
 * pmm_free_page
 * -------------
 * Bebaskan satu halaman fisik.
 *
 * Parameter:
 *   addr - Alamat fisik
 *
 * Return: Status operasi
 */
status_t pmm_free_page(alamat_fisik_t addr);

/*
 * pmm_alloc_pages
 * ---------------
 * Alokasikan multiple halaman.
 *
 * Parameter:
 *   count - Jumlah halaman
 *
 * Return: Alamat fisik halaman pertama, atau 0 jika gagal
 */
alamat_fisik_t pmm_alloc_pages(tak_bertanda32_t count);

/*
 * pmm_free_pages
 * --------------
 * Bebaskan multiple halaman.
 *
 * Parameter:
 *   addr  - Alamat fisik
 *   count - Jumlah halaman
 *
 * Return: Status operasi
 */
status_t pmm_free_pages(alamat_fisik_t addr, tak_bertanda32_t count);

/*
 * pmm_get_stats
 * -------------
 * Dapatkan statistik PMM.
 *
 * Parameter:
 *   total   - Pointer untuk total memori
 *   free_p  - Pointer untuk memori bebas
 *   used    - Pointer untuk memori terpakai
 */
void pmm_get_stats(tak_bertanda64_t *total, tak_bertanda64_t *free_p,
                   tak_bertanda64_t *used);

/*
 * pmm_print_stats
 * ---------------
 * Print statistik PMM.
 */
void pmm_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI PAGING (PAGING FUNCTIONS)
 * ===========================================================================
 */

/*
 * paging_init
 * -----------
 * Inisialisasi sistem paging.
 *
 * Parameter:
 *   mem_size - Ukuran memori
 *
 * Return: Status operasi
 */
status_t paging_init(tak_bertanda64_t mem_size);

/*
 * paging_map_page
 * ---------------
 * Map satu halaman.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *   paddr - Alamat fisik
 *   flags - Flag mapping
 *
 * Return: Status operasi
 */
status_t paging_map_page(alamat_virtual_t vaddr, alamat_fisik_t paddr,
                         tak_bertanda32_t flags);

/*
 * paging_unmap_page
 * -----------------
 * Unmap satu halaman.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Status operasi
 */
status_t paging_unmap_page(alamat_virtual_t vaddr);

/*
 * paging_get_physical
 * -------------------
 * Dapatkan alamat fisik dari virtual.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Alamat fisik, atau 0 jika tidak mapped
 */
alamat_fisik_t paging_get_physical(alamat_virtual_t vaddr);

/*
 * paging_is_mapped
 * ----------------
 * Cek apakah halaman sudah di-map.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: BENAR jika sudah mapped
 */
bool_t paging_is_mapped(alamat_virtual_t vaddr);

/*
 * paging_create_address_space
 * ---------------------------
 * Buat address space baru.
 *
 * Return: Pointer ke page_directory, atau NULL jika gagal
 */
struct page_directory *paging_create_address_space(void);

/*
 * paging_destroy_address_space
 * ----------------------------
 * Hancurkan address space.
 *
 * Parameter:
 *   dir - Pointer ke page_directory
 *
 * Return: Status operasi
 */
status_t paging_destroy_address_space(struct page_directory *dir);

/*
 * paging_switch_directory
 * -----------------------
 * Switch ke page directory lain.
 *
 * Parameter:
 *   dir - Pointer ke page_directory
 *
 * Return: Status operasi
 */
status_t paging_switch_directory(struct page_directory *dir);

/*
 * paging_get_current_directory
 * ----------------------------
 * Dapatkan page directory saat ini.
 *
 * Return: Pointer ke page_directory
 */
struct page_directory *paging_get_current_directory(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI HEAP (HEAP FUNCTIONS)
 * ===========================================================================
 */

/*
 * heap_init
 * ---------
 * Inisialisasi heap.
 *
 * Parameter:
 *   start_addr - Alamat awal
 *   size       - Ukuran heap
 *
 * Return: Status operasi
 */
status_t heap_init(void *start_addr, ukuran_t size);

/*
 * kmalloc
 * -------
 * Alokasikan memori dari heap.
 *
 * Parameter:
 *   size - Ukuran memori
 *
 * Return: Pointer ke memori, atau NULL jika gagal
 */
void *kmalloc(ukuran_t size);

/*
 * kfree
 * -----
 * Bebaskan memori.
 *
 * Parameter:
 *   ptr - Pointer ke memori
 */
void kfree(void *ptr);

/*
 * krealloc
 * --------
 * Realokasi memori.
 *
 * Parameter:
 *   ptr  - Pointer ke memori lama
 *   size - Ukuran baru
 *
 * Return: Pointer ke memori baru, atau NULL jika gagal
 */
void *krealloc(void *ptr, ukuran_t size);

/*
 * kcalloc
 * -------
 * Alokasi memori dan clear.
 *
 * Parameter:
 *   num  - Jumlah elemen
 *   size - Ukuran setiap elemen
 *
 * Return: Pointer ke memori, atau NULL jika gagal
 */
void *kcalloc(ukuran_t num, ukuran_t size);

/*
 * kmalloc_aligned
 * ---------------
 * Alokasi memori dengan alignment.
 *
 * Parameter:
 *   size    - Ukuran memori
 *   align   - Alignment
 *
 * Return: Pointer ke memori, atau NULL jika gagal
 */
void *kmalloc_aligned(ukuran_t size, ukuran_t align);

/*
 * heap_validate
 * -------------
 * Validasi heap.
 *
 * Return: BENAR jika heap valid
 */
bool_t heap_validate(void);

/*
 * heap_print_stats
 * ----------------
 * Print statistik heap.
 */
void heap_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI PROSES (PROCESS FUNCTIONS)
 * ===========================================================================
 * CATATAN: Fungsi-fungsi proses dideklarasikan secara lengkap di proses/proses.h
 * Deklarasi di bawah adalah untuk kemudahan akses dari kernel.h
 * Gunakan nama fungsi dalam Bahasa Indonesia untuk konsistensi.
 */

/* Inisialisasi subsistem proses */
status_t proses_init(void);

/* Buat proses baru (alias: proses_buat) */
pid_t proses_buat(const char *nama, pid_t ppid, tak_bertanda32_t flags);

/* Exit proses */
status_t proses_exit(pid_t pid, tanda32_t exit_code);

/* Cari proses berdasarkan PID */
proses_t *proses_cari(pid_t pid);

/* Dapatkan proses saat ini (alias: proses_dapat_saat_ini) */
proses_t *proses_dapat_saat_ini(void);

/* Set proses saat ini */
void proses_set_saat_ini(proses_t *proses);

/* Dapatkan kernel process */
proses_t *proses_dapat_kernel(void);

/* Dapatkan jumlah proses */
tak_bertanda32_t proses_dapat_jumlah(void);

/* Wait untuk proses child */
pid_t proses_wait(pid_t pid, tanda32_t *status);

/* Kill proses */
status_t proses_kill(pid_t pid, tak_bertanda32_t signal);

/* Print list proses */
void proses_print_list(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INTERUPSI (INTERRUPT FUNCTIONS)
 * ===========================================================================
 */

/*
 * interupsi_init
 * --------------
 * Inisialisasi sistem interupsi.
 *
 * Return: Status operasi
 */
status_t interupsi_init(void);

/*
 * interupsi_enable
 * ----------------
 * Aktifkan interupsi.
 */
void interupsi_enable(void);

/*
 * interupsi_disable
 * -----------------
 * Nonaktifkan interupsi.
 */
void interupsi_disable(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI MULTIBOOT (MULTIBOOT FUNCTIONS)
 * ===========================================================================
 */

/*
 * multiboot_parse
 * ---------------
 * Parse informasi multiboot.
 *
 * Parameter:
 *   magic    - Magic number
 *   bootinfo - Pointer ke boot info
 *
 * Return: Status operasi
 */
status_t multiboot_parse(tak_bertanda32_t magic, multiboot_info_t *bootinfo);

/*
 * multiboot_get_mem_lower
 * -----------------------
 * Dapatkan memori lower (dalam KB).
 *
 * Parameter:
 *   bootinfo - Pointer ke boot info
 *
 * Return: Memori lower dalam KB
 */
tak_bertanda32_t multiboot_get_mem_lower(multiboot_info_t *bootinfo);

/*
 * multiboot_get_mem_upper
 * -----------------------
 * Dapatkan memori upper (dalam KB).
 *
 * Parameter:
 *   bootinfo - Pointer ke boot info
 *
 * Return: Memori upper dalam KB
 */
tak_bertanda32_t multiboot_get_mem_upper(multiboot_info_t *bootinfo);

/*
 * multiboot_get_total_mem
 * -----------------------
 * Dapatkan total memori (dalam byte).
 *
 * Parameter:
 *   bootinfo - Pointer ke boot info
 *
 * Return: Total memori dalam byte
 */
tak_bertanda64_t multiboot_get_total_mem(multiboot_info_t *bootinfo);

/*
 * multiboot_get_cmdline
 * ---------------------
 * Dapatkan command line kernel.
 *
 * Return: String command line
 */
const char *multiboot_get_cmdline(void);

/*
 * multiboot_get_bootloader
 * ------------------------
 * Dapatkan nama bootloader.
 *
 * Return: String nama bootloader
 */
const char *multiboot_get_bootloader(void);

/*
 * multiboot_get_module_count
 * --------------------------
 * Dapatkan jumlah module.
 *
 * Return: Jumlah module
 */
tak_bertanda32_t multiboot_get_module_count(void);

/*
 * multiboot_get_module
 * --------------------
 * Dapatkan informasi module.
 *
 * Parameter:
 *   index - Index module
 *   mulai - Pointer untuk alamat awal
 *   akhir - Pointer untuk alamat akhir
 *
 * Return: Status operasi
 */
status_t multiboot_get_module(tak_bertanda32_t index,
                              alamat_fisik_t *mulai,
                              alamat_fisik_t *akhir);

/*
 * multiboot_get_module_string
 * ---------------------------
 * Dapatkan string module.
 *
 * Parameter:
 *   index - Index module
 *
 * Return: String module
 */
const char *multiboot_get_module_string(tak_bertanda32_t index);

/*
 * multiboot_get_mmap_count
 * ------------------------
 * Dapatkan jumlah memory map entry.
 *
 * Return: Jumlah entry
 */
tak_bertanda32_t multiboot_get_mmap_count(void);

/*
 * multiboot_get_mmap_entry
 * ------------------------
 * Dapatkan memory map entry.
 *
 * Parameter:
 *   index - Index entry
 *   entry - Pointer untuk hasil
 *
 * Return: Status operasi
 */
status_t multiboot_get_mmap_entry(tak_bertanda32_t index, mmap_entry_t *entry);

/*
 * multiboot_print_info
 * --------------------
 * Print informasi multiboot.
 *
 * Parameter:
 *   bootinfo - Pointer ke boot info
 */
void multiboot_print_info(multiboot_info_t *bootinfo);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI STACK (STACK FUNCTIONS)
 * ===========================================================================
 */

/*
 * setup_kernel_stack
 * ------------------
 * Setup stack kernel.
 *
 * Return: Status operasi
 */
status_t setup_kernel_stack(void);

/*
 * setup_kernel_stack_early
 * ------------------------
 * Setup stack kernel awal.
 *
 * Parameter:
 *   stack_top - Alamat top stack
 *   size      - Ukuran stack
 *
 * Return: Status operasi
 */
status_t setup_kernel_stack_early(void *stack_top, ukuran_t size);

/*
 * get_kernel_stack_top
 * --------------------
 * Dapatkan alamat top stack kernel.
 *
 * Return: Pointer ke top stack
 */
void *get_kernel_stack_top(void);

/*
 * get_kernel_stack_bottom
 * -----------------------
 * Dapatkan alamat bottom stack kernel.
 *
 * Return: Pointer ke bottom stack
 */
void *get_kernel_stack_bottom(void);

/*
 * get_kernel_stack_size
 * ---------------------
 * Dapatkan ukuran stack kernel.
 *
 * Return: Ukuran stack
 */
ukuran_t get_kernel_stack_size(void);

/*
 * check_stack_overflow
 * --------------------
 * Cek apakah stack overflow terjadi.
 *
 * Return: BENAR jika overflow
 */
bool_t check_stack_overflow(void);

/*
 * get_stack_usage
 * ---------------
 * Dapatkan penggunaan stack.
 *
 * Return: Jumlah byte yang digunakan
 */
ukuran_t get_stack_usage(void);

/*
 * get_stack_free
 * --------------
 * Dapatkan sisa stack yang tersedia.
 *
 * Return: Jumlah byte tersedia
 */
ukuran_t get_stack_free(void);

/*
 * print_stack_info
 * ----------------
 * Print informasi stack.
 */
void print_stack_info(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI INIT (INITIALIZATION FUNCTIONS)
 * ===========================================================================
 */

/*
 * kernel_init_early
 * -----------------
 * Inisialisasi awal kernel.
 *
 * Return: Status operasi
 */
status_t kernel_init_early(void);

/*
 * kernel_init_core
 * ----------------
 * Inisialisasi core subsistem.
 *
 * Return: Status operasi
 */
status_t kernel_init_core(void);

/*
 * kernel_init_drivers
 * -------------------
 * Inisialisasi driver.
 *
 * Return: Status operasi
 */
status_t kernel_init_drivers(void);

/*
 * kernel_init_services
 * --------------------
 * Inisialisasi layanan kernel.
 *
 * Return: Status operasi
 */
status_t kernel_init_services(void);

/*
 * kernel_init_late
 * ----------------
 * Inisialisasi akhir.
 *
 * Return: Status operasi
 */
status_t kernel_init_late(void);

/*
 * kernel_register_init
 * --------------------
 * Registrasi subsistem untuk inisialisasi.
 *
 * Parameter:
 *   nama     - Nama subsistem
 *   init     - Fungsi inisialisasi
 *   stage    - Stage inisialisasi
 *   required - Apakah required
 *
 * Return: Status operasi
 */
status_t kernel_register_init(const char *nama,
                              status_t (*init)(void),
                              tak_bertanda8_t stage,
                              bool_t required);

/*
 * kernel_get_init_status
 * ----------------------
 * Dapatkan status inisialisasi.
 *
 * Return: BENAR jika sudah diinisialisasi
 */
bool_t kernel_get_init_status(void);

/*
 * kernel_get_initialized_count
 * ----------------------------
 * Dapatkan jumlah subsistem yang diinisialisasi.
 *
 * Return: Jumlah subsistem
 */
tak_bertanda32_t kernel_get_initialized_count(void);

/*
 * kernel_print_init_status
 * ------------------------
 * Print status inisialisasi semua subsistem.
 */
void kernel_print_init_status(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI VIRTUAL MEMORY (VIRTUAL MEMORY FUNCTIONS)
 * ===========================================================================
 */

/*
 * virtual_init
 * ------------
 * Inisialisasi virtual memory manager.
 *
 * Return: Status operasi
 */
status_t virtual_init(void);

/*
 * vm_print_stats
 * --------------
 * Print statistik virtual memory.
 */
void vm_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI KMAP (KERNEL MAPPING FUNCTIONS)
 * ===========================================================================
 */

/*
 * kmap_init
 * ---------
 * Inisialisasi kernel mapping system.
 *
 * Return: Status operasi
 */
status_t kmap_init(void);

/*
 * kmap
 * ----
 * Map halaman fisik ke alamat virtual kernel.
 *
 * Parameter:
 *   phys - Alamat fisik
 *
 * Return: Alamat virtual, atau NULL jika gagal
 */
void *kmap(alamat_fisik_t phys);

/*
 * kunmap
 * ------
 * Unmap alamat virtual kernel.
 *
 * Parameter:
 *   vaddr - Alamat virtual
 *
 * Return: Status operasi
 */
status_t kunmap(void *vaddr);

/*
 * kmap_print_stats
 * ----------------
 * Print statistik kmap.
 */
void kmap_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI DMA (DMA BUFFER FUNCTIONS)
 * ===========================================================================
 */

/*
 * dma_init
 * --------
 * Inisialisasi DMA buffer manager.
 *
 * Return: Status operasi
 */
status_t dma_init(void);

/*
 * dma_print_stats
 * ---------------
 * Print statistik DMA.
 */
void dma_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI ALLOCATOR (SLAB ALLOCATOR FUNCTIONS)
 * ===========================================================================
 */

/*
 * allocator_init
 * --------------
 * Inisialisasi slab allocator.
 *
 * Return: Status operasi
 */
status_t allocator_init(void);

/*
 * allocator_print_stats
 * ---------------------
 * Print statistik allocator.
 */
void allocator_print_stats(void);

/*
 * ===========================================================================
 * INLINE ASSEMBLY HELPERS - x86/x86_64
 * ===========================================================================
 */

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

/* Hentikan CPU */
#define cpu_halt() \
    do { __asm__ __volatile__("hlt"); } while (0)

/* Disable interrupt */
#define cpu_disable_irq() \
    do { __asm__ __volatile__("cli"); } while (0)

/* Enable interrupt */
#define cpu_enable_irq() \
    do { __asm__ __volatile__("sti"); } while (0)

/* Tidak melakukan apa-apa */
#define cpu_nop() \
    do { __asm__ __volatile__("nop"); } while (0)

/* Invalidate TLB untuk satu halaman */
#define cpu_invlpg(addr) \
    do { __asm__ __volatile__("invlpg (%0)" : : "r"(addr) : "memory"); } while (0)

/* Reload CR3 */
#define cpu_reload_cr3() \
    do { \
        tak_bertanda32_t _cr3; \
        __asm__ __volatile__("mov %%cr3, %0\n\t" \
                             "mov %0, %%cr3" \
                             : "=r"(_cr3) :: "memory"); \
    } while (0)

/* Baca CR2 */
#define cpu_read_cr2() \
    ({ tak_bertanda32_t _val; \
       __asm__ __volatile__("mov %%cr2, %0" : "=r"(_val)); \
       _val; })

/* Baca CR3 */
#define cpu_read_cr3() \
    ({ tak_bertanda32_t _val; \
       __asm__ __volatile__("mov %%cr3, %0" : "=r"(_val)); \
       _val; })

/* Tulis CR3 */
#define cpu_write_cr3(val) \
    do { __asm__ __volatile__("mov %0, %%cr3" : : "r"(val) : "memory"); } while (0)

/* Baca CR0 */
#define cpu_read_cr0() \
    ({ tak_bertanda32_t _val; \
       __asm__ __volatile__("mov %%cr0, %0" : "=r"(_val)); \
       _val; })

/* Tulis CR0 */
#define cpu_write_cr0(val) \
    do { __asm__ __volatile__("mov %0, %%cr0" : : "r"(val) : "memory"); } while (0)

/* Baca byte dari port I/O */
#define inb(port) \
    ({ tak_bertanda8_t _val; \
       __asm__ __volatile__("inb %w1, %0" : "=a"(_val) : "Nd"((tak_bertanda16_t)(port))); \
       _val; })

/* Tulis byte ke port I/O */
#define outb(port, val) \
    do { __asm__ __volatile__("outb %b0, %w1" : : "a"((tak_bertanda8_t)(val)), "Nd"((tak_bertanda16_t)(port))); } while (0)

/* Baca word dari port I/O */
#define inw(port) \
    ({ tak_bertanda16_t _val; \
       __asm__ __volatile__("inw %w1, %0" : "=a"(_val) : "Nd"((tak_bertanda16_t)(port))); \
       _val; })

/* Tulis word ke port I/O */
#define outw(port, val) \
    do { __asm__ __volatile__("outw %w0, %w1" : : "a"((tak_bertanda16_t)(val)), "Nd"((tak_bertanda16_t)(port))); } while (0)

/* Baca dword dari port I/O */
#define inl(port) \
    ({ tak_bertanda32_t _val; \
       __asm__ __volatile__("inl %w1, %0" : "=a"(_val) : "Nd"((tak_bertanda16_t)(port))); \
       _val; })

/* Tulis dword ke port I/O */
#define outl(port, val) \
    do { __asm__ __volatile__("outl %0, %w1" : : "a"((tak_bertanda32_t)(val)), "Nd"((tak_bertanda16_t)(port))); } while (0)

/* IO delay */
#define io_delay() outb(0x80, 0)

/* Simpan flags dan disable interrupt */
#define cpu_save_flags() \
    ({ tak_bertanda32_t _flags; \
       __asm__ __volatile__("pushfl\n\t" \
                            "popl %0\n\t" \
                            "cli" \
                            : "=rm"(_flags) :: "memory"); \
       _flags; })

/* Restore flags */
#define cpu_restore_flags(flags) \
    do { __asm__ __volatile__("pushl %0\n\t" \
                              "popfl" \
                              : : "g"(flags) : "memory", "cc"); } while (0)

/* Makro helper untuk interrupt */
#define IRQ_DISABLE_SAVE(flags) \
    do { (flags) = cpu_save_flags(); } while (0)

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

#endif /* ARSITEKTUR_X86 || ARSITEKTUR_X86_64 */

/*
 * ===========================================================================
 * INLINE ASSEMBLY HELPERS - ARM
 * ===========================================================================
 */

#if defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7) || defined(ARSITEKTUR_ARM64)

/* Hentikan CPU */
#define cpu_halt() \
    do { __asm__ __volatile__("wfi"); } while (0)

/* Enable interrupt */
#define cpu_enable_irq() \
    do { __asm__ __volatile__("cpsie i"); } while (0)

/* Disable interrupt */
#define cpu_disable_irq() \
    do { __asm__ __volatile__("cpsid i"); } while (0)

/* Tidak melakukan apa-apa */
#define cpu_nop() \
    do { __asm__ __volatile__("nop"); } while (0)

/* Data synchronization barrier */
#define cpu_dsb() \
    do { __asm__ __volatile__("dsb sy"); } while (0)

/* Data memory barrier */
#define cpu_dmb() \
    do { __asm__ __volatile__("dmb sy"); } while (0)

/* Instruction synchronization barrier */
#define cpu_isb() \
    do { __asm__ __volatile__("isb sy"); } while (0)

#endif /* ARSITEKTUR_ARM || ARSITEKTUR_ARMV7 || ARSITEKTUR_ARM64 */

#endif /* INTI_KERNEL_H */
