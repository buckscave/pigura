/*
 * PIGURA OS - USER_MODE_ARMV7.C
 * ------------------------------
 * Implementasi operasi user mode untuk ARMv7.
 *
 * Berkas ini berisi fungsi-fungsi untuk switch ke user mode dan
 * menjalankan proses user pada prosesor ARMv7.
 *
 * Arsitektur: ARMv7 (Cortex-A series)
 * Versi: 1.0
 */

#include "../../../inti/types.h"
#include "../../../inti/konstanta.h"
#include "../../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* CPSR mode bits */
#define CPSR_MODE_USR       0x10
#define CPSR_MODE_FIQ       0x11
#define CPSR_MODE_IRQ       0x12
#define CPSR_MODE_SVC       0x13
#define CPSR_MODE_ABT       0x17
#define CPSR_MODE_UND       0x1B
#define CPSR_MODE_SYS       0x1F

/* CPSR flags */
#define CPSR_THUMB          (1 << 5)
#define CPSR_FIQ_DIS        (1 << 6)
#define CPSR_IRQ_DIS        (1 << 7)
#define CPSR_ASYNC_ABORT    (1 << 8)

/* User stack default */
#define USER_STACK_TOP      0xBFFF0000
#define USER_STACK_SIZE     (1 * 1024 * 1024)   /* 1 MB */

/* User heap default */
#define USER_HEAP_START     0x01000000
#define USER_HEAP_SIZE      (16 * 1024 * 1024)  /* 16 MB */

/*
 * ============================================================================
 * STRUKTUR DATA (DATA STRUCTURES)
 * ============================================================================
 */

/* User context structure */
typedef struct {
    tak_bertanda32_t r0;
    tak_bertanda32_t r1;
    tak_bertanda32_t r2;
    tak_bertanda32_t r3;
    tak_bertanda32_t r4;
    tak_bertanda32_t r5;
    tak_bertanda32_t r6;
    tak_bertanda32_t r7;
    tak_bertanda32_t r8;
    tak_bertanda32_t r9;
    tak_bertanda32_t r10;
    tak_bertanda32_t r11;
    tak_bertanda32_t r12;
    tak_bertanda32_t sp;
    tak_bertanda32_t lr;
    tak_bertanda32_t pc;
    tak_bertanda32_t cpsr;
} user_context_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Current user context */
static user_context_t g_user_context;

/* User mode flag */
static bool_t g_in_user_mode = SALAH;

/*
 * ============================================================================
 * FUNGSI BANTUAN CPSR (CPSR HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * user_get_cpsr
 * -------------
 * Baca nilai CPSR.
 *
 * Return: Nilai CPSR
 */
static inline tak_bertanda32_t user_get_cpsr(void)
{
    tak_bertanda32_t cpsr;
    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );
    return cpsr;
}

/*
 * user_set_cpsr
 * -------------
 * Set nilai CPSR.
 *
 * Parameter:
 *   cpsr - Nilai CPSR baru
 */
static inline void user_set_cpsr(tak_bertanda32_t cpsr)
{
    __asm__ __volatile__(
        "msr cpsr_c, %0"
        :
        : "r"(cpsr)
        : "memory"
    );
}

/*
 * user_get_spsr
 * -------------
 * Baca nilai SPSR.
 *
 * Return: Nilai SPSR
 */
static inline tak_bertanda32_t user_get_spsr(void)
{
    tak_bertanda32_t spsr;
    __asm__ __volatile__(
        "mrs %0, spsr"
        : "=r"(spsr)
    );
    return spsr;
}

/*
 * user_set_spsr
 * -------------
 * Set nilai SPSR.
 *
 * Parameter:
 *   spsr - Nilai SPSR baru
 */
static inline void user_set_spsr(tak_bertanda32_t spsr)
{
    __asm__ __volatile__(
        "msr spsr_c, %0"
        :
        : "r"(spsr)
        : "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI SWITCH KE USER MODE (USER MODE SWITCH FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_user_enter
 * --------------
 * Masuk ke user mode dan mulai eksekusi.
 *
 * Parameter:
 *   entry_point - Entry point user program
 *   stack_ptr   - Stack pointer user
 *
 * Return: Tidak return jika berhasil
 */
status_t hal_user_enter(tak_bertanda32_t entry_point, tak_bertanda32_t stack_ptr)
{
    tak_bertanda32_t cpsr;

    /* Save current context (kernel) */
    /* ... context saving dilakukan oleh scheduler ... */

    /* Setup user CPSR */
    /* User mode, IRQ dan FIQ enabled */
    cpsr = CPSR_MODE_USR;

    /* Setup user context */
    g_user_context.pc = entry_point;
    g_user_context.sp = stack_ptr;
    g_user_context.lr = 0;          /* No return address */
    g_user_context.cpsr = cpsr;

    /* Clear other registers */
    g_user_context.r0 = 0;
    g_user_context.r1 = 0;
    g_user_context.r2 = 0;
    g_user_context.r3 = 0;
    g_user_context.r4 = 0;
    g_user_context.r5 = 0;
    g_user_context.r6 = 0;
    g_user_context.r7 = 0;
    g_user_context.r8 = 0;
    g_user_context.r9 = 0;
    g_user_context.r10 = 0;
    g_user_context.r11 = 0;
    g_user_context.r12 = 0;

    g_in_user_mode = BENAR;

    /* Switch to user mode */
    /* Ini dilakukan dengan assembly untuk memastikan atomicity */
    __asm__ __volatile__(
        /* Save kernel LR */
        "mov    r8, lr\n"

        /* Set SPSR ke user mode CPSR */
        "msr    spsr_c, %0\n"

        /* Load user stack pointer */
        "mov    sp, %1\n"

        /* Load user PC ke LR */
        "mov    lr, %2\n"

        /* Switch ke user mode via exception return */
        "movs   pc, lr\n"

        :
        : "r"(cpsr), "r"(stack_ptr), "r"(entry_point)
        : "r8", "lr", "memory"
    );

    /* Should not reach here */
    return STATUS_BERHASIL;
}

/*
 * hal_user_return_to_kernel
 * -------------------------
 * Return dari user mode ke kernel.
 *
 * Parameter:
 *   return_value - Nilai return dari user program
 */
void hal_user_return_to_kernel(tak_bertanda32_t return_value)
{
    tak_bertanda32_t cpsr;

    /* Switch ke SVC mode */
    cpsr = user_get_cpsr();
    cpsr = (cpsr & ~0x1F) | CPSR_MODE_SVC | CPSR_IRQ_DIS;
    user_set_cpsr(cpsr);

    g_in_user_mode = SALAH;

    /* Return value ada di r0 */
    /* Scheduler akan mengambil alih */
}

/*
 * ============================================================================
 * FUNGSI ALAMAT USER (USER ADDRESS FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_user_alloc_stack
 * --------------------
 * Alokasikan stack untuk user process.
 *
 * Parameter:
 *   top - Pointer untuk menyimpan alamat top of stack
 *
 * Return: Status operasi
 */
status_t hal_user_alloc_stack(tak_bertanda32_t *top)
{
    alamat_fisik_t phys;
    tak_bertanda32_t pages;
    tak_bertanda32_t i;
    tak_bertanda32_t vaddr;

    if (top == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Alokasikan halaman untuk stack */
    pages = USER_STACK_SIZE / UKURAN_HALAMAN;
    phys = hal_memori_phys_alloc_pages(pages);

    if (phys == 0) {
        return STATUS_MEMORI_HABIS;
    }

    /* Map halaman ke virtual address (guard page + stack) */
    vaddr = USER_STACK_TOP - USER_STACK_SIZE + UKURAN_HALAMAN;

    for (i = 0; i < pages - 1; i++) {
        status_t status;

        status = hal_memori_map_page((void *)vaddr,
            phys + i * UKURAN_HALAMAN,
            0x01 | 0x02 | 0x04);    /* RW, User accessible */

        if (status != STATUS_BERHASIL) {
            hal_memori_phys_free_pages(phys, pages);
            return status;
        }

        vaddr += UKURAN_HALAMAN;
    }

    /* Guard page tidak dimap untuk deteksi stack overflow */
    *top = USER_STACK_TOP;

    return STATUS_BERHASIL;
}

/*
 * hal_user_free_stack
 * -------------------
 * Bebaskan stack user.
 *
 * Parameter:
 *   top - Top of stack address
 *
 * Return: Status operasi
 */
status_t hal_user_free_stack(tak_bertanda32_t top)
{
    tak_bertanda32_t vaddr;
    tak_bertanda32_t pages;
    tak_bertanda32_t i;
    alamat_fisik_t phys;

    if (top != USER_STACK_TOP) {
        return STATUS_PARAM_INVALID;
    }

    /* Unmap dan free halaman */
    pages = USER_STACK_SIZE / UKURAN_HALAMAN;
    vaddr = USER_STACK_TOP - USER_STACK_SIZE + UKURAN_HALAMAN;

    for (i = 0; i < pages - 1; i++) {
        phys = hal_memori_virt_to_phys((void *)vaddr);
        hal_memori_unmap_page((void *)vaddr);
        if (phys != 0) {
            hal_memori_phys_free_page(phys);
        }
        vaddr += UKURAN_HALAMAN;
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI HEAP USER (USER HEAP FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_user_init_heap
 * ------------------
 * Inisialisasi heap untuk user process.
 *
 * Parameter:
 *   brk_start - Alamat awal brk
 *
 * Return: Status operasi
 */
status_t hal_user_init_heap(tak_bertanda32_t brk_start)
{
    /* Brk akan di-expand sesuai kebutuhan via syscall brk/sbrk */
    return STATUS_BERHASIL;
}

/*
 * hal_user_expand_heap
 * --------------------
 * Expand heap user.
 *
 * Parameter:
 *   new_brk - Alamat brk baru
 *
 * Return: Status operasi
 */
status_t hal_user_expand_heap(tak_bertanda32_t new_brk)
{
    tak_bertanda32_t old_brk;
    tak_bertanda32_t vaddr;
    alamat_fisik_t phys;

    /* Get current brk */
    old_brk = 0;  /* TODO: Get from process struct */

    /* Align new_brk ke halaman */
    new_brk = (new_brk + UKURAN_HALAMAN - 1) & ~(UKURAN_HALAMAN - 1);

    /* Map new pages */
    for (vaddr = old_brk; vaddr < new_brk; vaddr += UKURAN_HALAMAN) {
        phys = hal_memori_phys_alloc_page();
        if (phys == 0) {
            return STATUS_MEMORI_HABIS;
        }

        hal_memori_map_page((void *)vaddr, phys,
            0x01 | 0x02 | 0x04);    /* RW, User accessible */
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI VALIDASI ALAMAT (ADDRESS VALIDATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_user_is_valid_addr
 * ----------------------
 * Cek apakah alamat valid untuk user.
 *
 * Parameter:
 *   addr - Alamat yang dicek
 *
 * Return: BENAR jika valid
 */
bool_t hal_user_is_valid_addr(tak_bertanda32_t addr)
{
    /* User space: 4 MB to 3 GB */
    if (addr >= ALAMAT_USER_MULAI && addr < ALAMAT_KERNEL_MULAI) {
        return BENAR;
    }

    return SALAH;
}

/*
 * hal_user_is_valid_range
 * -----------------------
 * Cek apakah range alamat valid untuk user.
 *
 * Parameter:
 *   addr  - Alamat awal
 *   size  - Ukuran range
 *
 * Return: BENAR jika valid
 */
bool_t hal_user_is_valid_range(tak_bertanda32_t addr, ukuran_t size)
{
    tak_bertanda32_t end;

    /* Check overflow */
    if (addr + size < addr) {
        return SALAH;
    }

    end = addr + size;

    /* Check both start and end are valid */
    return hal_user_is_valid_addr(addr) && hal_user_is_valid_addr(end - 1);
}

/*
 * hal_user_is_readable
 * --------------------
 * Cek apakah alamat bisa dibaca user.
 *
 * Parameter:
 *   addr - Alamat yang dicek
 *
 * Return: BENAR jika readable
 */
bool_t hal_user_is_readable(tak_bertanda32_t addr)
{
    if (!hal_user_is_valid_addr(addr)) {
        return SALAH;
    }

    /* Check page table untuk read permission */
    /* TODO: Implementasi check page table */

    return BENAR;
}

/*
 * hal_user_is_writable
 * --------------------
 * Cek apakah alamat bisa ditulis user.
 *
 * Parameter:
 *   addr - Alamat yang dicek
 *
 * Return: BENAR jika writable
 */
bool_t hal_user_is_writable(tak_bertanda32_t addr)
{
    if (!hal_user_is_valid_addr(addr)) {
        return SALAH;
    }

    /* Check page table untuk write permission */
    /* TODO: Implementasi check page table */

    return BENAR;
}

/*
 * ============================================================================
 * FUNGSI COPY DATA (DATA COPY FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_user_copy_to_user
 * ---------------------
 * Copy data dari kernel ke user space.
 *
 * Parameter:
 *   dest - Alamat tujuan di user space
 *   src  - Alamat sumber di kernel space
 *   size - Ukuran data
 *
 * Return: Jumlah bytes yang dicopy
 */
ukuran_t hal_user_copy_to_user(void *dest, const void *src, ukuran_t size)
{
    tak_bertanda32_t addr;

    if (dest == NULL || src == NULL || size == 0) {
        return 0;
    }

    addr = (tak_bertanda32_t)(tak_bertanda32_t)dest;

    /* Validate destination */
    if (!hal_user_is_writable(addr)) {
        return 0;
    }

    if (!hal_user_is_valid_range(addr, size)) {
        return 0;
    }

    /* Copy data */
    kernel_memcpy(dest, src, size);

    return size;
}

/*
 * hal_user_copy_from_user
 * -----------------------
 * Copy data dari user space ke kernel.
 *
 * Parameter:
 *   dest - Alamat tujuan di kernel space
 *   src  - Alamat sumber di user space
 *   size - Ukuran data
 *
 * Return: Jumlah bytes yang dicopy
 */
ukuran_t hal_user_copy_from_user(void *dest, const void *src, ukuran_t size)
{
    tak_bertanda32_t addr;

    if (dest == NULL || src == NULL || size == 0) {
        return 0;
    }

    addr = (tak_bertanda32_t)(tak_bertanda32_t)src;

    /* Validate source */
    if (!hal_user_is_readable(addr)) {
        return 0;
    }

    if (!hal_user_is_valid_range(addr, size)) {
        return 0;
    }

    /* Copy data */
    kernel_memcpy(dest, src, size);

    return size;
}

/*
 * ============================================================================
 * FUNGSI STRING USER (USER STRING FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_user_strlen
 * ---------------
 * Hitung panjang string di user space.
 *
 * Parameter:
 *   str - Alamat string di user space
 *   max - Panjang maksimum
 *
 * Return: Panjang string, atau -1 jika error
 */
tanda32_t hal_user_strlen(const char *str, ukuran_t max)
{
    ukuran_t len;
    const char *p;

    if (str == NULL) {
        return -1;
    }

    /* Validate address */
    if (!hal_user_is_readable((tak_bertanda32_t)(tak_bertanda32_t)str)) {
        return -1;
    }

    /* Count length */
    p = str;
    len = 0;

    while (len < max && *p != '\0') {
        p++;
        len++;

        /* Check each address for safety */
        if (!hal_user_is_readable((tak_bertanda32_t)(tak_bertanda32_t)p)) {
            return -1;
        }
    }

    return (tanda32_t)len;
}

/*
 * hal_user_strncpy
 * ----------------
 * Copy string dari user space ke kernel.
 *
 * Parameter:
 *   dest - Buffer tujuan
 *   src  - String di user space
 *   max  - Ukuran maksimum
 *
 * Return: Status operasi
 */
status_t hal_user_strncpy(char *dest, const char *src, ukuran_t max)
{
    tanda32_t len;

    if (dest == NULL || src == NULL || max == 0) {
        return STATUS_PARAM_INVALID;
    }

    len = hal_user_strlen(src, max - 1);
    if (len < 0) {
        return STATUS_AKSES_DITOLAK;
    }

    hal_user_copy_from_user(dest, src, (ukuran_t)len + 1);
    dest[len] = '\0';

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_user_mode_init
 * ------------------
 * Inisialisasi subsistem user mode.
 *
 * Return: Status operasi
 */
status_t hal_user_mode_init(void)
{
    kernel_memset(&g_user_context, 0, sizeof(user_context_t));
    g_in_user_mode = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_user_mode_shutdown
 * ----------------------
 * Shutdown subsistem user mode.
 *
 * Return: Status operasi
 */
status_t hal_user_mode_shutdown(void)
{
    g_in_user_mode = SALAH;

    return STATUS_BERHASIL;
}

/*
 * hal_user_is_in_user_mode
 * ------------------------
 * Cek apakah sedang di user mode.
 *
 * Return: BENAR jika di user mode
 */
bool_t hal_user_is_in_user_mode(void)
{
    tak_bertanda32_t cpsr;

    cpsr = user_get_cpsr();

    return ((cpsr & 0x1F) == CPSR_MODE_USR) ? BENAR : SALAH;
}
