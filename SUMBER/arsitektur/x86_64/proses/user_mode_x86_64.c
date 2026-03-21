/*
 * PIGURA OS - USER MODE x86_64
 * -----------------------------
 * Implementasi switching ke user mode untuk x86_64.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk beralih
 * dari kernel mode ke user mode pada arsitektur x86_64.
 *
 * Arsitektur: x86_64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA
 * ============================================================================
 */

/* Segment selector */
#define SEG_KERNEL_CODE         0x08
#define SEG_KERNEL_DATA         0x10
#define SEG_USER_CODE           0x18
#define SEG_USER_DATA           0x20

/* RPL (Request Privilege Level) */
#define RPL_KERNEL              0x00
#define RPL_USER                0x03

/* Flag */
#define RFLAGS_IF               0x200

/*
 * ============================================================================
 * DEKLARASI EKSTERNAL
 * ============================================================================
 */

/* Entry point assembly */
extern void _jump_to_usermode_64(tak_bertanda64_t entry,
                                 tak_bertanda64_t stack);

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * user_mode_x86_64_enter
 * ----------------------
 * Memasuki user mode dari kernel mode.
 *
 * Parameter:
 *   entry_point - Entry point di user mode
 *   stack_ptr   - Stack pointer user
 */
void user_mode_x86_64_enter(tak_bertanda64_t entry_point,
                             tak_bertanda64_t stack_ptr)
{
    kernel_printf("[USER-x86_64] Masuk ke user mode...\n");
    kernel_printf("[USER-x86_64] Entry: 0x%016X\n", entry_point);
    kernel_printf("[USER-x86_64] Stack: 0x%016X\n", stack_ptr);

    /* Pastikan stack 16-byte aligned */
    stack_ptr &= ~0xFULL;

    /* Panggil entry assembly */
    _jump_to_usermode_64(entry_point, stack_ptr);

    /* Tidak akan mencapai sini */
}

/*
 * user_mode_x86_64_return
 * -----------------------
 * Kembali ke kernel mode dari user mode via syscall.
 *
 * Parameter:
 *   retval - Nilai return
 */
void user_mode_x86_64_return(tak_bertanda64_t retval)
{
    /* Syscall return diimplementasi di syscall_x86_64.S */
    (void)retval;
}

/*
 * user_mode_x86_64_setup_stack
 * ----------------------------
 * Setup stack awal untuk user mode.
 *
 * Parameter:
 *   stack_top - Alamat top stack
 *   argc      - Jumlah argumen
 *   argv      - Array argumen
 *   envp      - Array environment
 *
 * Return:
 *   Stack pointer yang sudah disetup
 */
tak_bertanda64_t user_mode_x86_64_setup_stack(tak_bertanda64_t stack_top,
                                               tak_bertanda32_t argc,
                                               char **argv,
                                               char **envp)
{
    tak_bertanda64_t *stack64;
    tak_bertanda32_t i;

    /* Align stack ke 16 byte */
    stack_top &= ~0xFULL;

    /* Stack grows downward */
    stack64 = (tak_bertanda64_t *)stack_top;

    /* Push environment variables (NULL terminated) */
    if (envp != NULL) {
        tak_bertanda32_t envc = 0;

        /* Hitung jumlah env */
        while (envp[envc] != NULL) {
            envc++;
        }

        /* Push NULL terminator */
        *--stack64 = 0;

        /* Push env strings (reverse order) */
        for (i = envc; i > 0; i--) {
            *--stack64 = (tak_bertanda64_t)envp[i - 1];
        }
    } else {
        *--stack64 = 0;
    }

    /* Push argument strings (NULL terminated) */
    if (argv != NULL && argc > 0) {
        /* Push NULL terminator */
        *--stack64 = 0;

        /* Push argv strings (reverse order) */
        for (i = argc; i > 0; i--) {
            *--stack64 = (tak_bertanda64_t)argv[i - 1];
        }
    } else {
        *--stack64 = 0;
    }

    /* Push argc */
    *--stack64 = argc;

    /* Push fake return address (should never return) */
    *--stack64 = 0;

    return (tak_bertanda64_t)stack64;
}

/*
 * user_mode_x86_64_is_user_address
 * --------------------------------
 * Mengecek apakah alamat berada di ruang user.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return:
 *   BENAR jika alamat user
 */
bool_t user_mode_x86_64_is_user_address(tak_bertanda64_t addr)
{
    /*
     * Pada x86_64 dengan higher-half kernel:
     * - Alamat user: 0x0000000000000000 - 0x00007FFFFFFFFFFF
     * - Alamat kernel: 0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF
     */
    return (addr < 0xFFFF800000000000ULL) ? BENAR : SALAH;
}

/*
 * user_mode_x86_64_is_kernel_address
 * ----------------------------------
 * Mengecek apakah alamat berada di ruang kernel.
 *
 * Parameter:
 *   addr - Alamat virtual
 *
 * Return:
 *   BENAR jika alamat kernel
 */
bool_t user_mode_x86_64_is_kernel_address(tak_bertanda64_t addr)
{
    return (addr >= 0xFFFF800000000000ULL) ? BENAR : SALAH;
}

/*
 * user_mode_x86_64_validate_user_pointer
 * --------------------------------------
 * Memvalidasi pointer user.
 *
 * Parameter:
 *   ptr      - Pointer user
 *   size     - Ukuran data
 *   writable - Apakah perlu akses tulis
 *
 * Return:
 *   STATUS_BERHASIL jika valid
 */
status_t user_mode_x86_64_validate_user_pointer(const void *ptr,
                                                 ukuran_t size,
                                                 bool_t writable)
{
    tak_bertanda64_t addr = (tak_bertanda64_t)ptr;
    tak_bertanda64_t end;

    /* Cek overflow */
    if (size > 0 && addr > (addr + size - 1)) {
        return STATUS_PARAM_INVALID;
    }

    end = addr + size - 1;

    /* Cek apakah alamat di ruang user */
    if (!user_mode_x86_64_is_user_address(addr)) {
        return STATUS_AKSES_DITOLAK;
    }

    if (size > 0 && !user_mode_x86_64_is_user_address(end)) {
        return STATUS_AKSES_DITOLAK;
    }

    /* TODO: Cek apakah halaman sudah di-map dan memiliki permission */
    (void)writable;

    return STATUS_BERHASIL;
}

/*
 * user_mode_x86_64_validate_user_string
 * -------------------------------------
 * Memvalidasi string user.
 *
 * Parameter:
 *   str       - Pointer ke string
 *   max_len   - Panjang maksimum
 *
 * Return:
 *   STATUS_BERHASIL jika valid
 */
status_t user_mode_x86_64_validate_user_string(const char *str,
                                                ukuran_t max_len)
{
    tak_bertanda64_t addr = (tak_bertanda64_t)str;
    ukuran_t len = 0;

    /* Cek alamat awal */
    if (!user_mode_x86_64_is_user_address(addr)) {
        return STATUS_AKSES_DITOLAK;
    }

    /* Hitung panjang string dengan batas */
    while (len < max_len) {
        /* Cek setiap halaman */
        if (!user_mode_x86_64_is_user_address(addr + len)) {
            return STATUS_AKSES_DITOLAK;
        }

        if (str[len] == '\0') {
            return STATUS_BERHASIL;
        }

        len++;
    }

    /* String terlalu panjang */
    return STATUS_PARAM_INVALID;
}
