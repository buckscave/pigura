/*
 * PIGURA OS - SETUP_STACK.C
 * -------------------------
 * Implementasi setup stack kernel.
 *
 * Berkas ini berisi fungsi-fungsi untuk menginisialisasi stack
 * kernel pada berbagai arsitektur.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "../kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Ukuran stack kernel default */
#define STACK_KERNEL_SIZE       (8 * 1024)  /* 8 KB */

/* Stack magic untuk deteksi overflow */
#define STACK_MAGIC             0xDEADBEEF

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Stack kernel (aligned ke 16 byte) */
static tak_bertanda8_t kernel_stack[STACK_KERNEL_SIZE]
    __attribute__((aligned(16)));

/* Stack pointer kernel */
static void *kernel_esp = NULL;

/* Status stack */
static bool_t stack_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * stack_fill_magic
 * ----------------
 * Isi stack dengan magic value untuk deteksi overflow.
 */
static void stack_fill_magic(void)
{
    tak_bertanda32_t *ptr;
    tak_bertanda32_t i;

    ptr = (tak_bertanda32_t *)kernel_stack;

    for (i = 0; i < STACK_KERNEL_SIZE / sizeof(tak_bertanda32_t); i++) {
        ptr[i] = STACK_MAGIC;
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * setup_kernel_stack
 * ------------------
 * Setup stack kernel.
 * Fungsi ini menginisialisasi stack kernel dan mengatur stack pointer.
 *
 * Return: Status operasi
 */
status_t setup_kernel_stack(void)
{
    if (stack_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Isi stack dengan magic untuk deteksi overflow */
    stack_fill_magic();

    /* Set stack pointer ke alamat tertinggi dalam array */
    /* Stack tumbuh ke bawah, jadi ESP = base + size */
    kernel_esp = (void *)((alamat_ptr_t)kernel_stack + STACK_KERNEL_SIZE);

    /* Load stack pointer */
    /* Ini dilakukan dalam assembly untuk keamanan */
    __asm__ __volatile__(
        "mov %0, %%esp\n\t"
        "mov %0, %%ebp\n\t"
        :
        : "r"(kernel_esp)
        : "esp", "ebp"
    );

    stack_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * setup_kernel_stack_early
 * ------------------------
 * Setup stack kernel awal (untuk penggunaan sangat awal).
 * Dipanggil sebelum subsistem lain diinisialisasi.
 *
 * Parameter:
 *   stack_top - Alamat top stack
 *   size      - Ukuran stack
 *
 * Return: Status operasi
 */
status_t setup_kernel_stack_early(void *stack_top, ukuran_t size)
{
    if (stack_top == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }

    /* Set stack pointer */
    kernel_esp = stack_top;

    /* Load stack pointer */
    __asm__ __volatile__(
        "mov %0, %%esp\n\t"
        "mov %0, %%ebp\n\t"
        :
        : "r"(kernel_esp)
        : "esp", "ebp"
    );

    stack_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * get_kernel_stack_top
 * --------------------
 * Dapatkan alamat top stack kernel.
 *
 * Return: Alamat top stack
 */
void *get_kernel_stack_top(void)
{
    return kernel_esp;
}

/*
 * get_kernel_stack_bottom
 * -----------------------
 * Dapatkan alamat bottom stack kernel.
 *
 * Return: Alamat bottom stack
 */
void *get_kernel_stack_bottom(void)
{
    return (void *)kernel_stack;
}

/*
 * get_kernel_stack_size
 * ---------------------
 * Dapatkan ukuran stack kernel.
 *
 * Return: Ukuran stack dalam byte
 */
ukuran_t get_kernel_stack_size(void)
{
    return STACK_KERNEL_SIZE;
}

/*
 * check_stack_overflow
 * --------------------
 * Cek apakah stack overflow terjadi.
 *
 * Return: BENAR jika overflow terdeteksi
 */
bool_t check_stack_overflow(void)
{
    tak_bertanda32_t *ptr;
    tak_bertanda32_t i;

    /* Cek magic value di awal stack */
    ptr = (tak_bertanda32_t *)kernel_stack;

    for (i = 0; i < 4; i++) {
        if (ptr[i] != STACK_MAGIC) {
            return BENAR;
        }
    }

    return SALAH;
}

/*
 * get_stack_usage
 * ---------------
 * Dapatkan penggunaan stack.
 *
 * Return: Jumlah byte yang digunakan
 */
ukuran_t get_stack_usage(void)
{
    tak_bertanda32_t *ptr;
    tak_bertanda32_t i;
    ukuran_t used;

    ptr = (tak_bertanda32_t *)kernel_stack;
    used = 0;

    /* Hitung berapa banyak magic yang tertimpa */
    for (i = 0; i < STACK_KERNEL_SIZE / sizeof(tak_bertanda32_t); i++) {
        if (ptr[i] != STACK_MAGIC) {
            used++;
        }
    }

    return used * sizeof(tak_bertanda32_t);
}

/*
 * get_stack_free
 * --------------
 * Dapatkan sisa stack yang tersedia.
 *
 * Return: Jumlah byte yang masih tersedia
 */
ukuran_t get_stack_free(void)
{
    return STACK_KERNEL_SIZE - get_stack_usage();
}

/*
 * print_stack_info
 * ----------------
 * Print informasi stack.
 */
void print_stack_info(void)
{
    kernel_printf("\n[STACK] Informasi Stack Kernel:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Top:    0x%08lX\n", (alamat_ptr_t)kernel_esp);
    kernel_printf("  Bottom: 0x%08lX\n", (alamat_ptr_t)kernel_stack);
    kernel_printf("  Size:   %lu bytes\n", (tak_bertanda64_t)STACK_KERNEL_SIZE);
    kernel_printf("  Used:   %lu bytes\n",
                  (tak_bertanda64_t)get_stack_usage());
    kernel_printf("  Free:   %lu bytes\n",
                  (tak_bertanda64_t)get_stack_free());

    if (check_stack_overflow()) {
        kernel_set_color(VGA_MERAH, VGA_HITAM);
        kernel_printf("  STATUS: OVERFLOW TERDETEKSI!\n");
        kernel_set_color(VGA_ABU_ABU, VGA_HITAM);
    } else {
        kernel_set_color(VGA_HIJAU, VGA_HITAM);
        kernel_printf("  STATUS: OK\n");
        kernel_set_color(VGA_ABU_ABU, VGA_HITAM);
    }

    kernel_printf("========================================\n\n");
}

/*
 * stack_guard_check
 * -----------------
 * Cek stack guard page.
 * Digunakan untuk proteksi stack overflow.
 *
 * Return: BENAR jika guard page terinjak
 */
bool_t stack_guard_check(void)
{
    /* Untuk implementasi dengan paging, cek guard page */
    /* Untuk sekarang, gunakan magic value check */
    return check_stack_overflow();
}
