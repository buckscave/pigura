/*
 * PIGURA OS - RING_SWITCH.C
 * --------------------------
 * Implementasi transisi antar privilege level (ring switching).
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani transisi antara
 * berbagai privilege level CPU (Ring 0-3) pada arsitektur x86/x86_64.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi diimplementasikan lengkap
 * - Batas 80 karakter per baris
 *
 * Versi: 3.0
 * Tanggal: 2025
 */

#include "../kernel.h"

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

#include "proses.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Selector segment dengan RPL (Requested Privilege Level) */
#define RING0_CODE_SELECTOR     SELECTOR_KERNEL_CODE
#define RING0_DATA_SELECTOR     SELECTOR_KERNEL_DATA
#define RING3_CODE_SELECTOR     (SELECTOR_USER_CODE | 0x03)
#define RING3_DATA_SELECTOR     (SELECTOR_USER_DATA | 0x03)

/* EFLAGS bit */
#define EFLAGS_IF               0x00000200
#define EFLAGS_IOPL_MASK        0x00003000
#define EFLAGS_IOPL_SHIFT       12
#define EFLAGS_NT               0x00004000
#define EFLAGS_RF               0x00010000
#define EFLAGS_VM               0x00020000
#define EFLAGS_AC               0x00040000
#define EFLAGS_ID               0x00200000

/* EFLAGS default untuk user mode */
#define EFLAGS_USER_DEFAULT     (EFLAGS_IF | (0 << EFLAGS_IOPL_SHIFT))

/* EFLAGS default untuk kernel mode */
#define EFLAGS_KERNEL_DEFAULT   (EFLAGS_IF | (0 << EFLAGS_IOPL_SHIFT))

/* Stack alignment */
#define STACK_ALIGN             16

/* CPL mask dari selector */
#define CPL_MASK                0x03

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik ring switch */
static struct {
    tak_bertanda64_t ring0_to_ring3;
    tak_bertanda64_t ring3_to_ring0;
    tak_bertanda64_t ring3_to_ring3;
    tak_bertanda64_t invalid_attempts;
    tak_bertanda64_t security_violations;
} ring_stats = {0, 0, 0, 0, 0};

/* Status inisialisasi */
static bool_t ring_initialized = SALAH;

/* Lock */
static spinlock_t ring_lock;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * get_current_cpl
 * ---------------
 * Dapatkan Current Privilege Level dari CS register.
 *
 * Return: CPL (0-3)
 */
static tak_bertanda32_t get_current_cpl(void)
{
    tak_bertanda32_t cs;
    
    __asm__ __volatile__(
        "mov %%cs, %0"
        : "=r"(cs)
    );
    
    return cs & CPL_MASK;
}

/*
 * is_kernel_mode
 * --------------
 * Cek apakah CPU sedang di kernel mode (Ring 0).
 *
 * Return: BENAR jika di kernel mode
 */
static bool_t is_kernel_mode(void)
{
    return get_current_cpl() == 0;
}

/*
 * is_user_mode
 * ------------
 * Cek apakah CPU sedang di user mode (Ring 3).
 *
 * Return: BENAR jika di user mode
 */
static bool_t is_user_mode(void)
{
    return get_current_cpl() == 3;
}

/*
 * set_segment_selectors
 * ---------------------
 * Set semua segment selector ke nilai yang sesuai privilege.
 *
 * Parameter:
 *   privilege - Target privilege level
 */
static void set_segment_selectors(tak_bertanda32_t privilege)
{
    tak_bertanda16_t data_sel;
    
    if (privilege == 0) {
        data_sel = RING0_DATA_SELECTOR;
    } else {
        data_sel = RING3_DATA_SELECTOR;
    }
    
    __asm__ __volatile__(
        "mov %0, %%ds\n\t"
        "mov %0, %%es\n\t"
        "mov %0, %%fs\n\t"
        "mov %0, %%gs\n\t"
        :
        : "r"(data_sel)
        : "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * ring_switch_init
 * ----------------
 * Inisialisasi subsistem ring switch.
 *
 * Return: Status operasi
 */
status_t ring_switch_init(void)
{
    if (ring_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    spinlock_init(&ring_lock);
    
    kernel_memset(&ring_stats, 0, sizeof(ring_stats));
    
    ring_initialized = BENAR;
    
    kernel_printf("[RING] Ring switch initialized (CPL: %lu)\n",
                  (tak_bertanda64_t)get_current_cpl());
    
    return STATUS_BERHASIL;
}

/*
 * switch_ke_user_mode
 * -------------------
 * Switch dari kernel mode ke user mode.
 *
 * Parameter:
 *   entry - Entry point program user
 *   stack - Pointer ke stack user
 *   argc  - Jumlah argumen
 *   argv  - Pointer ke array argumen
 */
void switch_ke_user_mode(void *entry, void *stack,
                         tak_bertanda32_t argc, tak_bertanda32_t argv)
{
    tak_bertanda32_t *stack_ptr;
    tak_bertanda32_t eflags;
    tak_bertanda32_t current_cpl;
    
    if (!ring_initialized) {
        PANIC("switch_ke_user_mode: not initialized");
    }
    
    /* Cek apakah sudah di user mode */
    current_cpl = get_current_cpl();
    if (current_cpl != 0) {
        ring_stats.security_violations++;
        PANIC("switch_ke_user_mode: not in kernel mode");
    }
    
    /* Align stack */
    stack_ptr = (tak_bertanda32_t *)stack;
    stack_ptr = (tak_bertanda32_t *)((alamat_ptr_t)stack_ptr &
                                      ~(STACK_ALIGN - 1));
    
    /* Setup stack untuk entry user mode */
    stack_ptr--;
    *stack_ptr = argv;
    stack_ptr--;
    *stack_ptr = argc;
    
    /* Setup EFLAGS untuk user mode */
    eflags = EFLAGS_USER_DEFAULT;
    
    /* Update statistik */
    ring_stats.ring0_to_ring3++;
    
    /* Disable interrupt sebelum switch */
    cpu_disable_irq();
    
    /* Set segment selectors ke user mode */
    set_segment_selectors(3);
    
    /* Lakukan switch ke user mode menggunakan IRET */
    __asm__ __volatile__(
        "pushl %0\n\t"          /* SS */
        "pushl %1\n\t"          /* ESP */
        "pushl %2\n\t"          /* EFLAGS */
        "pushl %3\n\t"          /* CS */
        "pushl %4\n\t"          /* EIP */
        "iret\n\t"
        :
        : "r"((tak_bertanda32_t)RING3_DATA_SELECTOR),
          "r"((tak_bertanda32_t)(alamat_ptr_t)stack_ptr),
          "r"(eflags),
          "r"((tak_bertanda32_t)RING3_CODE_SELECTOR),
          "r"((tak_bertanda32_t)(alamat_ptr_t)entry)
        : "memory"
    );
    
    /* Tidak seharusnya sampai di sini */
    PANIC("switch_ke_user_mode: should not return");
}

/*
 * return_ke_kernel
 * ----------------
 * Return ke kernel mode.
 */
void return_ke_kernel(void)
{
    proses_t *proses;
    thread_t *thread;
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL || proses->main_thread == NULL) {
        return;
    }
    
    thread = proses->main_thread;
    
    /* Update statistik */
    ring_stats.ring3_to_ring0++;
    
    /* Set segment selectors ke kernel mode */
    set_segment_selectors(0);
    
    /* Return melalui context */
    context_return_to_user(thread->context);
}

/*
 * validasi_pointer_user
 * ---------------------
 * Validasi pointer dari user space.
 *
 * Parameter:
 *   ptr - Pointer yang divalidasi
 *
 * Return: BENAR jika valid
 */
bool_t validasi_pointer_user(const void *ptr)
{
    alamat_virtual_t addr;
    proses_t *proses;
    
    if (ptr == NULL) {
        return SALAH;
    }
    
    addr = (alamat_virtual_t)(alamat_ptr_t)ptr;
    
    /* Harus di user space */
    if (addr < ALAMAT_USER_MULAI || addr >= ALAMAT_USER_AKHIR) {
        return SALAH;
    }
    
    /* Cek mapping */
    proses = proses_dapat_saat_ini();
    if (proses == NULL || proses->vm == NULL) {
        return SALAH;
    }
    
    return vm_query(proses->vm, addr, NULL, NULL, NULL);
}

/*
 * validasi_string_user
 * --------------------
 * Validasi string dari user space.
 *
 * Parameter:
 *   str - Pointer ke string
 *
 * Return: BENAR jika valid
 */
bool_t validasi_string_user(const char *str)
{
    alamat_virtual_t addr;
    proses_t *proses;
    ukuran_t len;
    ukuran_t max_len;
    
    if (str == NULL) {
        return SALAH;
    }
    
    addr = (alamat_virtual_t)(alamat_ptr_t)str;
    
    /* Harus di user space */
    if (addr < ALAMAT_USER_MULAI || addr >= ALAMAT_USER_AKHIR) {
        return SALAH;
    }
    
    /* Cek mapping */
    proses = proses_dapat_saat_ini();
    if (proses == NULL || proses->vm == NULL) {
        return SALAH;
    }
    
    /* Cek setiap halaman yang dilalui string */
    max_len = ALAMAT_USER_AKHIR - addr;
    len = 0;
    
    while (len < max_len) {
        alamat_virtual_t page_addr;
        
        page_addr = RATAKAN_BAWAH(addr + len, UKURAN_HALAMAN);
        
        if (!vm_query(proses->vm, page_addr, NULL, NULL, NULL)) {
            return SALAH;
        }
        
        /* Cek null terminator */
        if (str[len] == '\0') {
            return BENAR;
        }
        
        len++;
    }
    
    return SALAH;
}

/*
 * validasi_array_user
 * -------------------
 * Validasi array dari user space.
 *
 * Parameter:
 *   arr       - Pointer ke array
 *   elem_size - Ukuran setiap elemen
 *   count     - Jumlah elemen
 *
 * Return: BENAR jika valid
 */
bool_t validasi_array_user(const void *arr, ukuran_t elem_size,
                            ukuran_t count)
{
    alamat_virtual_t addr;
    alamat_virtual_t end;
    proses_t *proses;
    
    if (arr == NULL || elem_size == 0 || count == 0) {
        return SALAH;
    }
    
    addr = (alamat_virtual_t)(alamat_ptr_t)arr;
    
    /* Cek overflow */
    if (addr + (elem_size * count) < addr) {
        return SALAH;
    }
    
    end = addr + (elem_size * count);
    
    /* Harus di user space */
    if (addr < ALAMAT_USER_MULAI || end > ALAMAT_USER_AKHIR) {
        return SALAH;
    }
    
    /* Cek mapping */
    proses = proses_dapat_saat_ini();
    if (proses == NULL || proses->vm == NULL) {
        return SALAH;
    }
    
    /* Cek halaman pertama dan terakhir */
    if (!vm_query(proses->vm, RATAKAN_BAWAH(addr, UKURAN_HALAMAN),
                  NULL, NULL, NULL)) {
        return SALAH;
    }
    
    if (!vm_query(proses->vm, RATAKAN_BAWAH(end - 1, UKURAN_HALAMAN),
                  NULL, NULL, NULL)) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * copy_from_user
 * --------------
 * Copy data dari user space dengan validasi.
 *
 * Parameter:
 *   dest - Buffer kernel
 *   src  - Buffer user
 *   size - Ukuran data
 *
 * Return: Status operasi
 */
status_t copy_from_user(void *dest, const void *src, ukuran_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    if (!validasi_array_user(src, 1, size)) {
        return STATUS_AKSES_DITOLAK;
    }
    
    kernel_memcpy(dest, src, size);
    
    return STATUS_BERHASIL;
}

/*
 * copy_to_user
 * ------------
 * Copy data ke user space dengan validasi.
 *
 * Parameter:
 *   dest - Buffer user
 *   src  - Buffer kernel
 *   size - Ukuran data
 *
 * Return: Status operasi
 */
status_t copy_to_user(void *dest, const void *src, ukuran_t size)
{
    if (dest == NULL || src == NULL || size == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    if (!validasi_array_user(dest, 1, size)) {
        return STATUS_AKSES_DITOLAK;
    }
    
    kernel_memcpy(dest, src, size);
    
    return STATUS_BERHASIL;
}

/*
 * copy_string_from_user
 * ---------------------
 * Copy string dari user space dengan validasi.
 *
 * Parameter:
 *   dest    - Buffer kernel
 *   src     - String user
 *   max_len - Panjang maksimum
 *
 * Return: Status operasi
 */
status_t copy_string_from_user(char *dest, const char *src,
                               ukuran_t max_len)
{
    ukuran_t len;
    
    if (dest == NULL || src == NULL || max_len == 0) {
        return STATUS_PARAM_INVALID;
    }
    
    if (!validasi_string_user(src)) {
        return STATUS_AKSES_DITOLAK;
    }
    
    len = kernel_strlen(src);
    if (len >= max_len) {
        len = max_len - 1;
    }
    
    kernel_strncpy(dest, src, len);
    dest[len] = '\0';
    
    return STATUS_BERHASIL;
}

/*
 * ring_dapat_cpl
 * -------------
 * Dapatkan Current Privilege Level.
 *
 * Return: CPL (0-3)
 */
tak_bertanda32_t ring_dapat_cpl(void)
{
    return get_current_cpl();
}

/*
 * ring_is_kernel_mode
 * -------------------
 * Cek apakah sedang di kernel mode.
 *
 * Return: BENAR jika di kernel mode
 */
bool_t ring_is_kernel_mode(void)
{
    return is_kernel_mode();
}

/*
 * ring_is_user_mode
 * -----------------
 * Cek apakah sedang di user mode.
 *
 * Return: BENAR jika di user mode
 */
bool_t ring_is_user_mode(void)
{
    return is_user_mode();
}

/*
 * ring_dapat_statistik
 * --------------------
 * Dapatkan statistik ring switch.
 *
 * Parameter:
 *   to_user    - Pointer untuk jumlah switch ke user mode
 *   to_kernel  - Pointer untuk jumlah switch ke kernel mode
 *   invalid    - Pointer untuk jumlah attempt invalid
 *   violations - Pointer untuk jumlah pelanggaran keamanan
 */
void ring_dapat_statistik(tak_bertanda64_t *to_user,
                          tak_bertanda64_t *to_kernel,
                          tak_bertanda64_t *invalid,
                          tak_bertanda64_t *violations)
{
    if (to_user != NULL) {
        *to_user = ring_stats.ring0_to_ring3;
    }
    
    if (to_kernel != NULL) {
        *to_kernel = ring_stats.ring3_to_ring0;
    }
    
    if (invalid != NULL) {
        *invalid = ring_stats.invalid_attempts;
    }
    
    if (violations != NULL) {
        *violations = ring_stats.security_violations;
    }
}

/*
 * ring_print_stats
 * ----------------
 * Print statistik ring switch.
 */
void ring_print_stats(void)
{
    kernel_printf("\n[RING] Statistik Ring Switch:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Ring 0 -> Ring 3:    %lu\n",
                  ring_stats.ring0_to_ring3);
    kernel_printf("  Ring 3 -> Ring 0:    %lu\n",
                  ring_stats.ring3_to_ring0);
    kernel_printf("  Ring 3 -> Ring 3:    %lu\n",
                  ring_stats.ring3_to_ring3);
    kernel_printf("  Invalid Attempts:    %lu\n",
                  ring_stats.invalid_attempts);
    kernel_printf("  Security Violations: %lu\n",
                  ring_stats.security_violations);
    kernel_printf("  Current CPL:         %lu\n",
                  (tak_bertanda64_t)get_current_cpl());
    kernel_printf("========================================\n");
}

#endif /* ARSITEKTUR_X86 || ARSITEKTUR_X86_64 */
