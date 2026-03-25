/*
 * PIGURA OS - USER MODE x86
 * --------------------------
 * Implementasi fungsi-fungsi untuk user mode.
 *
 * Berkas ini berisi fungsi-fungsi untuk meluncurkan proses
 * user mode dan mengelola transisi kernel-user.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../../inti/kernel.h"

/* Forward declaration dari tss_x86.c */
extern void tss_set_esp0(tak_bertanda32_t esp0);

/*
 * ============================================================================
 * KONSTANTA
 * ============================================================================
 */

/* Selector */
#define USER_CS                0x1B
#define USER_DS                0x23

/* Stack alignment */
#define STACK_ALIGN            16

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* Stack kernel untuk user mode switch */
static tak_bertanda8_t g_user_stack_kernel[UKURAN_HALAMAN]
    __attribute__((aligned(UKURAN_HALAMAN)));

/* Flag user mode aktif */
static bool_t g_user_mode_aktif = SALAH;

/*
 * ============================================================================
 * FUNGSI PUBLIC
 * ============================================================================
 */

/*
 * user_mode_init
 * --------------
 * Menginisialisasi subsistem user mode.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t user_mode_init(void)
{
    kernel_memset(g_user_stack_kernel, 0, sizeof(g_user_stack_kernel));

    g_user_mode_aktif = SALAH;

    kernel_printf("[USER] Subsistem user mode diinisialisasi\n");

    return STATUS_BERHASIL;
}

/*
 * user_mode_luncurkan
 * -------------------
 * Meluncurkan proses pertama ke user mode.
 *
 * Parameter:
 *   entry_point - Alamat entry point
 *   stack       - Alamat stack user
 *   argumen     - Pointer ke argumen
 *
 * Return:
 *   Tidak return jika berhasil
 */
status_t user_mode_luncurkan(alamat_virtual_t entry_point,
                              alamat_virtual_t stack,
                              void *argumen)
{
    (void)argumen;  /* TODO: implementasi passing argumen */
    tak_bertanda32_t *stack_ptr;
    tak_bertanda32_t eflags;

    kernel_printf("[USER] Meluncurkan ke user mode: 0x%08x\n", 
                  entry_point);

    /* Align stack ke 16 byte */
    stack = (alamat_virtual_t)((tak_bertanda32_t)stack & ~0x0F);

    /* Setup user stack */
    stack_ptr = (tak_bertanda32_t *)stack;

    /* Push pseudo return address (exit syscall) */
    stack_ptr--;
    *stack_ptr = 0;  /* User akan exit jika return */

    /* Setup kernel stack untuk IRET */
    /* Format stack untuk IRET:
     *   [SS]
     *   [ESP]
     *   [EFLAGS]
     *   [CS]
     *   [EIP]
     */

    /* Disable interrupt sementara */
    cpu_disable_irq();

    /* Set TSS ESP0 */
    tss_set_esp0((tak_bertanda32_t)(uintptr_t)&g_user_stack_kernel[
                 sizeof(g_user_stack_kernel) - 4]);

    /* Baca EFLAGS */
    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0\n\t"
        : "=r"(eflags)
    );

    /* Enable interrupt di user mode */
    eflags |= 0x200;

    /* Setup segment register */
    __asm__ __volatile__(
        "movw %0, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        :
        : "i"(USER_DS)
        : "ax"
    );

    g_user_mode_aktif = BENAR;

    /* IRET ke user mode */
    __asm__ __volatile__(
        "pushl %0\n\t"          /* SS */
        "pushl %1\n\t"          /* ESP */
        "pushl %2\n\t"          /* EFLAGS */
        "pushl %3\n\t"          /* CS */
        "pushl %4\n\t"          /* EIP */
        "iret\n\t"
        :
        : "i"(USER_DS),
          "r"(stack_ptr),
          "r"(eflags),
          "i"(USER_CS),
          "r"(entry_point)
    );

    /* Tidak akan sampai sini */
    return STATUS_BERHASIL;
}

/*
 * user_mode_luncurkan_dengan_args
 * -------------------------------
 * Meluncurkan proses dengan argumen argc/argv.
 *
 * Parameter:
 *   entry - Entry point
 *   stack - Stack user
 *   argc  - Jumlah argumen
 *   argv  - Array argumen
 *
 * Return:
 *   Tidak return jika berhasil
 */
status_t user_mode_luncurkan_dengan_args(alamat_virtual_t entry,
                                          alamat_virtual_t stack,
                                          tak_bertanda32_t argc,
                                          char **argv)
{
    tak_bertanda32_t *stack_ptr;
    tak_bertanda32_t i;
    tak_bertanda32_t eflags;

    /* Align stack */
    stack = (alamat_virtual_t)((tak_bertanda32_t)stack & ~0x0F);
    stack_ptr = (tak_bertanda32_t *)stack;

    /* Push argv strings ke stack */
    /* Pertama, copy string ke stack */
    for (i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            ukuran_t len = kernel_strlen(argv[i]);
            stack_ptr = (tak_bertanda32_t *)((uintptr_t)stack_ptr -
                        len - 1);
            kernel_strncpy((char *)stack_ptr, argv[i], len + 1);
            /* Update argv[i] ke alamat baru */
            argv[i] = (char *)stack_ptr;
        }
    }

    /* Align lagi */
    stack_ptr = (tak_bertanda32_t *)((uintptr_t)stack_ptr & ~0x03);

    /* Push NULL terminator untuk argv */
    stack_ptr--;
    *stack_ptr = 0;

    /* Push argv pointers */
    for (i = argc; i > 0; i--) {
        stack_ptr--;
        *stack_ptr = (tak_bertanda32_t)(uintptr_t)argv[i - 1];
    }

    /* Push argv pointer */
    tak_bertanda32_t argv_ptr = (tak_bertanda32_t)(uintptr_t)stack_ptr;
    stack_ptr--;
    *stack_ptr = argv_ptr;

    /* Push argc */
    stack_ptr--;
    *stack_ptr = argc;

    /* Push return address (0 = exit) */
    stack_ptr--;
    *stack_ptr = 0;

    /* Disable interrupt */
    cpu_disable_irq();

    /* Set TSS ESP0 */
    tss_set_esp0((tak_bertanda32_t)(uintptr_t)&g_user_stack_kernel[
                 sizeof(g_user_stack_kernel) - 4]);

    /* Get EFLAGS */
    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0\n\t"
        : "=r"(eflags)
    );
    eflags |= 0x200;

    /* Set segment */
    __asm__ __volatile__(
        "movw %0, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        :
        : "i"(USER_DS)
        : "ax"
    );

    g_user_mode_aktif = BENAR;

    /* IRET */
    __asm__ __volatile__(
        "pushl %0\n\t"
        "pushl %1\n\t"
        "pushl %2\n\t"
        "pushl %3\n\t"
        "pushl %4\n\t"
        "iret\n\t"
        :
        : "i"(USER_DS),
          "r"(stack_ptr),
          "r"(eflags),
          "i"(USER_CS),
          "r"(entry)
    );

    return STATUS_BERHASIL;
}

/*
 * user_mode_is_active
 * -------------------
 * Cek apakah user mode aktif.
 *
 * Return:
 *   BENAR jika ada proses user yang berjalan
 */
bool_t user_mode_is_active(void)
{
    return g_user_mode_aktif;
}

/*
 * user_mode_get_kernel_stack
 * --------------------------
 * Mendapatkan alamat stack kernel untuk proses user.
 *
 * Return:
 *   Pointer ke stack kernel
 */
void *user_mode_get_kernel_stack(void)
{
    return &g_user_stack_kernel[sizeof(g_user_stack_kernel) - 4];
}

/*
 * user_mode_set_kernel_stack
 * --------------------------
 * Mengatur stack kernel untuk proses user.
 *
 * Parameter:
 *   stack - Pointer ke stack kernel
 */
void user_mode_set_kernel_stack(void *stack)
{
    if (stack != NULL) {
        tss_set_esp0((tak_bertanda32_t)(uintptr_t)stack);
    }
}

/*
 * user_mode_return_ke_kernel
 * --------------------------
 * Kembali ke kernel dari user mode (exit).
 *
 * Parameter:
 *   exit_code - Kode exit
 */
void user_mode_return_ke_kernel(tanda32_t exit_code)
{
    g_user_mode_aktif = SALAH;

    /* Syscall exit */
    kernel_printf("[USER] Proses exit dengan kode: %d\n", exit_code);

    /* Halt */
    while (1) {
        cpu_halt();
    }
}

/*
 * user_mode_setup_stack
 * ---------------------
 * Setup stack untuk proses user baru.
 *
 * Parameter:
 *   stack_top  - Alamat atas stack
 *   entry      - Entry point
 *   argumen    - Pointer ke argumen
 *
 * Return:
 *   Alamat stack pointer yang siap digunakan
 */
alamat_virtual_t user_mode_setup_stack(alamat_virtual_t stack_top,
                                        alamat_virtual_t entry,
                                        void *argumen)
{
    (void)entry;  /* Entry point tidak digunakan di setup stack */
    tak_bertanda32_t *sp;

    /* Align ke 16 byte */
    stack_top = (alamat_virtual_t)((tak_bertanda32_t)stack_top & ~0x0F);
    sp = (tak_bertanda32_t *)stack_top;

    /* Push return address (exit) */
    sp--;
    *sp = 0;

    /* Push argumen jika ada */
    if (argumen != NULL) {
        sp--;
        *sp = (tak_bertanda32_t)(uintptr_t)argumen;
    }

    return (alamat_virtual_t)sp;
}
