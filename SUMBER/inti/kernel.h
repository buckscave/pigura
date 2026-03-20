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
 * FORWARD DECLARATIONS (FORWARD DECLARATIONS)
 * ============================================================================
 */

/* Forward declarations untuk menghindari circular dependency */
struct proses;
struct vm_descriptor;
struct page_directory;

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
 * TIPE STATUS PROSES (PROCESS STATUS TYPES)
 * ============================================================================
 */

/* Status proses */
typedef enum {
    PROSES_STATUS_BELUM = 0,        /* Belum diinisialisasi */
    PROSES_STATUS_SIAP,            /* Ready to run */
    PROSES_STATUS_JALAN,           /* Currently running */
    PROSES_STATUS_TUNGGU,          /* Waiting for event */
    PROSES_STATUS_ZOMBIE,          /* Terminated, not reaped */
    PROSES_STATUS_STOP             /* Stopped by signal */
} proses_status_t;

/* Prioritas proses */
typedef enum {
    PRIORITAS_RENDAH = 0,
    PRIORITAS_NORMAL = 1,
    PRIORITAS_TINGGI = 2,
    PRIORITAS_REALTIME = 3
} prioritas_t;

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
extern proses_t *g_proses_sekarang;

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
 * kernel_snprintf
 * ---------------
 * Format string ke buffer dengan batas ukuran.
 *
 * Parameter:
 *   str    - Buffer output
 *   size   - Ukuran buffer
 *   format - Format string
 *   ...    - Argumen format
 *
 * Return: Jumlah karakter yang diformat
 */
int kernel_snprintf(char *str, ukuran_t size, const char *format, ...);

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

/* Log level maksimum */
#define LOG_LEVEL   LOG_DEBUG

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
 * DEKLARASI FUNGSI PMM (PHYSICAL MEMORY MANAGER)
 * ============================================================================
 */

/*
 * pmm_init
 * --------
 * Inisialisasi physical memory manager.
 */
status_t pmm_init(tak_bertanda64_t mem_size, void *bitmap_addr);

/*
 * pmm_add_region
 * --------------
 * Tambah region memori.
 */
status_t pmm_add_region(alamat_fisik_t mulai, alamat_fisik_t akhir,
                        tak_bertanda32_t tipe);

/*
 * pmm_reserve
 * -----------
 * Reserve region memori.
 */
status_t pmm_reserve(alamat_fisik_t mulai, tak_bertanda64_t ukuran);

/*
 * pmm_alloc_page
 * --------------
 * Alokasikan satu halaman fisik.
 */
alamat_fisik_t pmm_alloc_page(void);

/*
 * pmm_free_page
 * -------------
 * Bebaskan satu halaman fisik.
 */
status_t pmm_free_page(alamat_fisik_t addr);

/*
 * pmm_alloc_pages
 * ---------------
 * Alokasikan multiple halaman.
 */
alamat_fisik_t pmm_alloc_pages(tak_bertanda32_t count);

/*
 * pmm_free_pages
 * --------------
 * Bebaskan multiple halaman.
 */
status_t pmm_free_pages(alamat_fisik_t addr, tak_bertanda32_t count);

/*
 * pmm_get_stats
 * -------------
 * Dapatkan statistik PMM.
 */
void pmm_get_stats(tak_bertanda64_t *total, tak_bertanda64_t *free,
                   tak_bertanda64_t *used);

/*
 * pmm_print_stats
 * ---------------
 * Print statistik PMM.
 */
void pmm_print_stats(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI PAGING (PAGING FUNCTIONS)
 * ============================================================================
 */

/*
 * paging_init
 * -----------
 * Inisialisasi sistem paging.
 */
status_t paging_init(tak_bertanda64_t mem_size);

/*
 * paging_map_page
 * ---------------
 * Map satu halaman.
 */
status_t paging_map_page(alamat_virtual_t vaddr, alamat_fisik_t paddr,
                         tak_bertanda32_t flags);

/*
 * paging_unmap_page
 * -----------------
 * Unmap satu halaman.
 */
status_t paging_unmap_page(alamat_virtual_t vaddr);

/*
 * paging_get_physical
 * -------------------
 * Dapatkan alamat fisik dari virtual.
 */
alamat_fisik_t paging_get_physical(alamat_virtual_t vaddr);

/*
 * paging_is_mapped
 * ----------------
 * Cek apakah halaman sudah di-map.
 */
bool_t paging_is_mapped(alamat_virtual_t vaddr);

/*
 * paging_create_address_space
 * ---------------------------
 * Buat address space baru.
 */
struct page_directory *paging_create_address_space(void);

/*
 * paging_destroy_address_space
 * ----------------------------
 * Hancurkan address space.
 */
status_t paging_destroy_address_space(struct page_directory *dir);

/*
 * paging_switch_directory
 * -----------------------
 * Switch ke page directory lain.
 */
status_t paging_switch_directory(struct page_directory *dir);

/*
 * ============================================================================
 * DEKLARASI FUNGSI HEAP (HEAP FUNCTIONS)
 * ============================================================================
 */

/*
 * heap_init
 * ---------
 * Inisialisasi heap.
 */
status_t heap_init(void *start_addr, ukuran_t size);

/*
 * kmalloc
 * -------
 * Alokasikan memori dari heap.
 */
void *kmalloc(ukuran_t size);

/*
 * kfree
 * -----
 * Bebaskan memori.
 */
void kfree(void *ptr);

/*
 * krealloc
 * --------
 * Realokasi memori.
 */
void *krealloc(void *ptr, ukuran_t size);

/*
 * kcalloc
 * -------
 * Alokasi memori dan clear.
 */
void *kcalloc(ukuran_t num, ukuran_t size);

/*
 * heap_validate
 * -------------
 * Validasi heap.
 */
bool_t heap_validate(void);

/*
 * heap_print_stats
 * ----------------
 * Print statistik heap.
 */
void heap_print_stats(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI VIRTUAL MEMORY (VIRTUAL MEMORY FUNCTIONS)
 * ============================================================================
 */

/* VMA flags */
#define VMA_FLAG_NONE           0x00
#define VMA_FLAG_READ           0x01
#define VMA_FLAG_WRITE          0x02
#define VMA_FLAG_EXEC           0x04
#define VMA_FLAG_STACK          0x08
#define VMA_FLAG_HEAP           0x10
#define VMA_FLAG_SHARED         0x20
#define VMA_FLAG_FIXED          0x40

/* VM descriptor type */
typedef struct vm_descriptor vm_descriptor_t;

/*
 * virtual_init
 * ------------
 * Inisialisasi virtual memory manager.
 */
status_t virtual_init(void);

/*
 * vm_create_address_space
 * -----------------------
 * Buat address space baru.
 */
vm_descriptor_t *vm_create_address_space(void);

/*
 * vm_destroy_address_space
 * ------------------------
 * Hancurkan address space.
 */
status_t vm_destroy_address_space(vm_descriptor_t *vm);

/*
 * vm_map
 * ------
 * Map region memori.
 */
alamat_virtual_t vm_map(vm_descriptor_t *vm, alamat_virtual_t start,
                        ukuran_t size, tak_bertanda32_t prot,
                        tak_bertanda32_t flags);

/*
 * vm_unmap
 * --------
 * Unmap region memori.
 */
status_t vm_unmap(vm_descriptor_t *vm, alamat_virtual_t start,
                  ukuran_t size);

/*
 * vm_brk
 * ------
 * Ubah break address.
 */
alamat_virtual_t vm_brk(vm_descriptor_t *vm, alamat_virtual_t addr);

/*
 * vm_handle_page_fault
 * --------------------
 * Handler page fault.
 */
bool_t vm_handle_page_fault(alamat_virtual_t addr,
                            tak_bertanda32_t error,
                            vm_descriptor_t *vm);

/*
 * vm_print_stats
 * --------------
 * Print statistik VM.
 */
void vm_print_stats(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI KMAP (KERNEL MAPPING FUNCTIONS)
 * ============================================================================
 */

/*
 * kmap_init
 * ---------
 * Inisialisasi kmap.
 */
status_t kmap_init(void);

/*
 * kmap
 * ----
 * Map halaman fisik ke virtual.
 */
void *kmap(alamat_fisik_t phys);

/*
 * kunmap
 * ------
 * Unmap alamat virtual.
 */
status_t kunmap(void *vaddr);

/*
 * kmap_print_stats
 * ----------------
 * Print statistik kmap.
 */
void kmap_print_stats(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI DMA (DMA BUFFER FUNCTIONS)
 * ============================================================================
 */

/* DMA buffer type */
typedef struct dma_buffer dma_buffer_t;

/*
 * dma_init
 * --------
 * Inisialisasi DMA buffer manager.
 */
status_t dma_init(void);

/*
 * dma_alloc_coherent
 * ------------------
 * Alokasikan DMA buffer coherent.
 */
dma_buffer_t *dma_alloc_coherent(ukuran_t size, tak_bertanda32_t align,
                                 const char *nama);

/*
 * dma_free_coherent
 * -----------------
 * Bebaskan DMA buffer.
 */
void dma_free_coherent(dma_buffer_t *buf);

/*
 * dma_print_stats
 * ---------------
 * Print statistik DMA.
 */
void dma_print_stats(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI ALLOCATOR (SLAB ALLOCATOR FUNCTIONS)
 * ============================================================================
 */

/*
 * allocator_init
 * --------------
 * Inisialisasi slab allocator.
 */
status_t allocator_init(void);

/*
 * allocator_print_stats
 * ---------------------
 * Print statistik allocator.
 */
void allocator_print_stats(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI MEMORI UTAMA (MAIN MEMORY FUNCTIONS)
 * ============================================================================
 */

/*
 * memori_init
 * -----------
 * Inisialisasi subsistem memori lengkap.
 */
status_t memori_init(multiboot_info_t *bootinfo);

/*
 * memori_get_total
 * ----------------
 * Dapatkan total memori fisik.
 */
tak_bertanda64_t memori_get_total(void);

/*
 * memori_get_available
 * --------------------
 * Dapatkan memori tersedia.
 */
tak_bertanda64_t memori_get_available(void);

/*
 * memori_print_info
 * -----------------
 * Print informasi memori.
 */
void memori_print_info(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI PROSES (PROCESS FUNCTIONS)
 * ============================================================================
 */

/* Process type */
typedef struct proses proses_t;

/* Invalid PID */
#define PID_INVALID             ((pid_t)-1)

/*
 * proses_init
 * -----------
 * Inisialisasi subsistem proses.
 */
status_t proses_init(void);

/*
 * proses_create
 * -------------
 * Buat proses baru.
 */
pid_t proses_create(const char *nama, pid_t ppid, tak_bertanda32_t flags);

/*
 * proses_exit
 * -----------
 * Exit proses.
 */
status_t proses_exit(pid_t pid, tanda32_t exit_code);

/*
 * proses_cari
 * -----------
 * Cari proses berdasarkan PID.
 */
proses_t *proses_cari(pid_t pid);

/*
 * proses_get_current
 * ------------------
 * Dapatkan proses saat ini.
 */
proses_t *proses_get_current(void);

/*
 * proses_set_current
 * ------------------
 * Set proses saat ini.
 */
void proses_set_current(proses_t *proses);

/*
 * proses_get_kernel
 * -----------------
 * Dapatkan kernel process.
 */
proses_t *proses_get_kernel(void);

/*
 * proses_get_count
 * ----------------
 * Dapatkan jumlah proses.
 */
tak_bertanda32_t proses_get_count(void);

/*
 * proses_wait
 * -----------
 * Wait untuk proses child.
 */
pid_t proses_wait(pid_t pid, tanda32_t *status);

/*
 * proses_kill
 * -----------
 * Kill proses.
 */
status_t proses_kill(pid_t pid, tak_bertanda32_t signal);

/*
 * proses_print_list
 * -----------------
 * Print list proses.
 */
void proses_print_list(void);

/*
 * ============================================================================
 * DEKLARASI FUNGSI INTERUPSI (INTERRUPT FUNCTIONS)
 * ============================================================================
 */

/*
 * interupsi_init
 * --------------
 * Inisialisasi sistem interupsi.
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
