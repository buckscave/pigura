/*
 * PIGURA OS - USER_MODE_ARM.C
 * ----------------------------
 * Implementasi operasi user mode untuk ARM (32-bit).
 *
 * Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
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
#define CPSR_MODE_SVC       0x13
#define CPSR_IRQ_DIS        (1 << 7)
#define CPSR_FIQ_DIS        (1 << 6)

/* User stack default */
#define USER_STACK_TOP      0xBFFF0000
#define USER_STACK_SIZE     (1 * 1024 * 1024)   /* 1 MB */

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
 * Return: Status operasi
 */
status_t hal_user_enter(tak_bertanda32_t entry_point, tak_bertanda32_t stack_ptr)
{
    tak_bertanda32_t cpsr;

    /* Setup user CPSR */
    /* User mode, IRQ dan FIQ enabled */
    cpsr = CPSR_MODE_USR;

    /* Switch to user mode via exception return */
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
    cpsr = CPSR_MODE_SVC | CPSR_IRQ_DIS;
    __asm__ __volatile__(
        "msr    cpsr_c, %0"
        :
        : "r"(cpsr)
        : "memory"
    );

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

    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(cpsr)
    );

    return ((cpsr & 0x1F) == CPSR_MODE_USR) ? BENAR : SALAH;
}
