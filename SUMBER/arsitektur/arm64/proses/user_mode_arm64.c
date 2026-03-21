/*
 * PIGURA OS - USER_MODE_ARM64.C
 * -----------------------------
 * Implementasi user mode support untuk ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola transisi
 * antara kernel mode dan user mode pada ARM64.
 *
 * Arsitektur: ARM64/AArch64 (64-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* SPSR values */
#define SPSR_EL0t               0x00000000
#define SPSR_EL1t               0x00000004
#define SPSR_EL1h               0x00000005
#define SPSR_EL2t               0x00000008
#define SPSR_EL2h               0x00000009
#define SPSR_EL3t               0x0000000C
#define SPSR_EL3h               0x0000000D

/* SPSR flags */
#define SPSR_N                  (1 << 31)
#define SPSR_Z                  (1 << 30)
#define SPSR_C                  (1 << 29)
#define SPSR_V                  (1 << 28)
#define SPSR_Q                  (1 << 27)
#define SPSR_SS                 (1 << 21)
#define SPSR_IL                 (1 << 20)
#define SPSR_D                  (1 << 9)
#define SPSR_A                  (1 << 8)
#define SPSR_I                  (1 << 7)
#define SPSR_F                  (1 << 6)

/* Stack size */
#define USER_STACK_SIZE         (256 * 1024)    /* 256KB */
#define USER_STACK_ALIGN        16

/* User memory layout */
#define USER_CODE_BASE          0x0000000000400000ULL
#define USER_DATA_BASE          0x0000000001000000ULL
#define USER_STACK_TOP          0x0000000008000000ULL
#define USER_HEAP_BASE          0x0000000010000000ULL

/*
 * ============================================================================
 * TIPE DATA LOKAL (LOCAL DATA TYPES)
 * ============================================================================
 */

/* User mode context */
typedef struct {
    tak_bertanda64_t regs[31];      /* x0-x30 */
    tak_bertanda64_t sp;
    tak_bertanda64_t pc;
    tak_bertanda64_t pstate;        /* SPSR */
} user_context_t;

/* User process state */
typedef struct {
    user_context_t context;
    tak_bertanda64_t entry;
    tak_bertanda64_t stack_top;
    tak_bertanda64_t stack_size;
    tak_bertanda64_t heap_base;
    tak_bertanda64_t heap_size;
    tak_bertanda64_t pgd;
    tak_bertanda16_t asid;
} user_process_t;

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Current user process */
static user_process_t *g_current_user = NULL;

/* ASID counter */
static tak_bertanda16_t g_user_asid_counter = 1;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * _get_asid
 * ---------
 * Allocate new ASID.
 */
static tak_bertanda16_t _get_asid(void)
{
    tak_bertanda16_t asid;

    asid = g_user_asid_counter++;

    /* ASID is 16-bit */
    if (g_user_asid_counter == 0) {
        g_user_asid_counter = 1;
    }

    return asid;
}

/*
 * _setup_user_stack
 * -----------------
 * Setup user stack.
 */
static tak_bertanda64_t _setup_user_stack(tak_bertanda64_t stack_top,
                                           tak_bertanda64_t argc,
                                           tak_bertanda64_t argv,
                                           tak_bertanda64_t envp)
{
    tak_bertanda64_t *sp;

    /* Align stack to 16 bytes */
    stack_top &= ~0xFULL;

    /* Reserve space on stack */
    sp = (tak_bertanda64_t *)stack_top;

    /* Push argc, argv, envp */
    sp[-1] = envp;
    sp[-2] = argv;
    sp[-3] = argc;

    /* Align to 16 bytes (AAPCS64 requirement) */
    return (tak_bertanda64_t)(sp - 3) & ~0xFULL;
}

/*
 * _enter_user_mode
 * ---------------
 * Enter user mode from kernel.
 */
static void _enter_user_mode(tak_bertanda64_t entry,
                              tak_bertanda64_t sp,
                              tak_bertanda64_t arg)
{
    /* Disable interrupts for setup */
    __asm__ __volatile__("msr DAIFSet, #0xF");

    /* Setup user stack */
    __asm__ __volatile__(
        "msr SP_EL0, %0"
        :
        : "r"(sp)
    );

    /* Setup entry point */
    __asm__ __volatile__(
        "msr ELR_EL1, %0"
        :
        : "r"(entry)
    );

    /* Setup SPSR for EL0t */
    __asm__ __volatile__(
        "msr SPSR_EL1, %0"
        :
        : "r"(SPSR_EL0t)
    );

    /* Set argument in x0 */
    __asm__ __volatile__(
        "mov x0, %0"
        :
        : "r"(arg)
    );

    /* Clear other registers */
    __asm__ __volatile__(
        "mov x1, xzr\n\t"
        "mov x2, xzr\n\t"
        "mov x3, xzr\n\t"
        "mov x4, xzr\n\t"
        "mov x5, xzr\n\t"
        "mov x6, xzr\n\t"
        "mov x7, xzr\n\t"
        "mov x8, xzr\n\t"
        "mov x9, xzr\n\t"
        "mov x10, xzr\n\t"
        "mov x11, xzr\n\t"
        "mov x12, xzr\n\t"
        "mov x13, xzr\n\t"
        "mov x14, xzr\n\t"
        "mov x15, xzr\n\t"
        "mov x16, xzr\n\t"
        "mov x17, xzr\n\t"
        "mov x18, xzr\n\t"
        "mov x19, xzr\n\t"
        "mov x20, xzr\n\t"
        "mov x21, xzr\n\t"
        "mov x22, xzr\n\t"
        "mov x23, xzr\n\t"
        "mov x24, xzr\n\t"
        "mov x25, xzr\n\t"
        "mov x26, xzr\n\t"
        "mov x27, xzr\n\t"
        "mov x28, xzr\n\t"
        "mov x29, xzr\n\t"
        "mov x30, xzr"
    );

    /* Barrier */
    __asm__ __volatile__("isb");

    /* Enable interrupts and enter user mode */
    __asm__ __volatile__(
        "msr DAIFClr, #0xF\n\t"
        "eret"
    );
}

/*
 * _return_from_user
 * -----------------
 * Return from user mode to kernel.
 */
static void _return_from_user(tak_bertanda64_t retval)
{
    /* This is called when user process exits via syscall */
    kernel_printf("[USER] Process returned with value: %ld\n",
        (tanda64_t)retval);

    /* Schedule next process */
    proses_yield();
}

/*
 * ============================================================================
 * PUBLIC FUNCTIONS
 * ============================================================================
 */

/*
 * user_mode_init
 * --------------
 * Initialize user mode support.
 */
status_t user_mode_init(void)
{
    kernel_printf("[USER-ARM64] Initializing user mode support\n");

    g_current_user = NULL;
    g_user_asid_counter = 1;

    return STATUS_BERHASIL;
}

/*
 * user_mode_create_process
 * ------------------------
 * Create new user process.
 */
status_t user_mode_create_process(pid_t pid, tak_bertanda64_t entry,
                                   tak_bertanda64_t stack_top)
{
    user_process_t *uproc;
    tak_bertanda64_t pgd;

    uproc = (user_process_t *)hal_memory_alloc_page();
    if (uproc == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_memset(uproc, 0, sizeof(user_process_t));

    /* Setup process */
    uproc->entry = entry;
    uproc->stack_top = stack_top;
    uproc->stack_size = USER_STACK_SIZE;
    uproc->heap_base = USER_HEAP_BASE;
    uproc->heap_size = 0;

    /* Allocate ASID */
    uproc->asid = _get_asid();

    /* Allocate page table */
    pgd = hal_memory_alloc_page();
    if (pgd == ALAMAT_FISIK_INVALID) {
        hal_memory_free_page(uproc);
        return STATUS_MEMORI_HABIS;
    }
    uproc->pgd = pgd;

    kernel_printf("[USER-ARM64] Created user process PID=%u ASID=%u\n",
        pid, uproc->asid);

    return STATUS_BERHASIL;
}

/*
 * user_mode_start
 * ---------------
 * Start user process.
 */
status_t user_mode_start(tak_bertanda64_t entry, tak_bertanda64_t sp,
                          tak_bertanda64_t arg)
{
    kernel_printf("[USER-ARM64] Starting user process at 0x%lX\n", entry);

    /* Enter user mode */
    _enter_user_mode(entry, sp, arg);

    /* Never reached */
    return STATUS_BERHASIL;
}

/*
 * user_mode_exit
 * --------------
 * Exit user process.
 */
void user_mode_exit(tak_bertanda32_t exit_code)
{
    kernel_printf("[USER-ARM64] Process exiting with code %u\n", exit_code);

    /* Clean up and schedule next */
    if (g_current_user != NULL) {
        /* Free page table */
        if (g_current_user->pgd != 0) {
            hal_memory_free_page((alamat_fisik_t)g_current_user->pgd);
        }
        hal_memory_free_page(g_current_user);
        g_current_user = NULL;
    }

    proses_yield();
}

/*
 * user_mode_get_sp
 * ----------------
 * Get user stack pointer.
 */
tak_bertanda64_t user_mode_get_sp(void)
{
    tak_bertanda64_t sp;

    __asm__ __volatile__(
        "mrs %0, SP_EL0"
        : "=r"(sp)
    );

    return sp;
}

/*
 * user_mode_set_sp
 * ----------------
 * Set user stack pointer.
 */
void user_mode_set_sp(tak_bertanda64_t sp)
{
    __asm__ __volatile__(
        "msr SP_EL0, %0"
        :
        : "r"(sp)
    );
}

/*
 * user_mode_get_pc
 * ----------------
 * Get user PC.
 */
tak_bertanda64_t user_mode_get_pc(void)
{
    tak_bertanda64_t pc;

    __asm__ __volatile__(
        "mrs %0, ELR_EL1"
        : "=r"(pc)
    );

    return pc;
}

/*
 * user_mode_set_pc
 * ----------------
 * Set user PC.
 */
void user_mode_set_pc(tak_bertanda64_t pc)
{
    __asm__ __volatile__(
        "msr ELR_EL1, %0"
        :
        : "r"(pc)
    );
}

/*
 * user_mode_get_spsr
 * ------------------
 * Get user SPSR.
 */
tak_bertanda64_t user_mode_get_spsr(void)
{
    tak_bertanda64_t spsr;

    __asm__ __volatile__(
        "mrs %0, SPSR_EL1"
        : "=r"(spsr)
    );

    return spsr;
}

/*
 * user_mode_set_spsr
 * ------------------
 * Set user SPSR.
 */
void user_mode_set_spsr(tak_bertanda64_t spsr)
{
    __asm__ __volatile__(
        "msr SPSR_EL1, %0"
        :
        : "r"(spsr)
    );
}

/*
 * user_mode_get_tpidr
 * -------------------
 * Get TLS pointer.
 */
tak_bertanda64_t user_mode_get_tpidr(void)
{
    tak_bertanda64_t tpidr;

    __asm__ __volatile__(
        "mrs %0, TPIDR_EL0"
        : "=r"(tpidr)
    );

    return tpidr;
}

/*
 * user_mode_set_tpidr
 * -------------------
 * Set TLS pointer.
 */
void user_mode_set_tpidr(tak_bertanda64_t tpidr)
{
    __asm__ __volatile__(
        "msr TPIDR_EL0, %0"
        :
        : "r"(tpidr)
    );
}

/*
 * user_mode_return_to_user
 * ------------------------
 * Return to user mode from kernel.
 */
void user_mode_return_to_user(tak_bertanda64_t pc, tak_bertanda64_t sp,
                               tak_bertanda64_t pstate)
{
    /* Setup PC */
    __asm__ __volatile__(
        "msr ELR_EL1, %0"
        :
        : "r"(pc)
    );

    /* Setup SP */
    __asm__ __volatile__(
        "msr SP_EL0, %0"
        :
        : "r"(sp)
    );

    /* Setup SPSR */
    __asm__ __volatile__(
        "msr SPSR_EL1, %0"
        :
        : "r"(pstate)
    );

    /* Barrier */
    __asm__ __volatile__("isb");

    /* Return to user */
    __asm__ __volatile__(
        "msr DAIFClr, #0xF\n\t"
        "eret"
    );
}

/*
 * user_mode_setup_args
 * --------------------
 * Setup user process arguments.
 */
tak_bertanda64_t user_mode_setup_args(tak_bertanda64_t stack_top,
                                       tak_bertanda32_t argc,
                                       char **argv,
                                       char **envp)
{
    return _setup_user_stack(stack_top, argc,
                             (tak_bertanda64_t)argv,
                             (tak_bertanda64_t)envp);
}

/*
 * user_mode_allocate_stack
 * ------------------------
 * Allocate user stack.
 */
tak_bertanda64_t user_mode_allocate_stack(void)
{
    void *stack;

    stack = hal_memory_alloc_pages(USER_STACK_SIZE / PAGE_SIZE);
    if (stack == NULL) {
        return 0;
    }

    return (tak_bertanda64_t)stack + USER_STACK_SIZE;
}

/*
 * user_mode_free_stack
 * --------------------
 * Free user stack.
 */
void user_mode_free_stack(tak_bertanda64_t stack_bottom)
{
    hal_memory_free_pages((alamat_fisik_t)stack_bottom,
                          USER_STACK_SIZE / PAGE_SIZE);
}

/*
 * user_mode_get_layout
 * --------------------
 * Get user memory layout.
 */
void user_mode_get_layout(tak_bertanda64_t *code_base,
                           tak_bertanda64_t *stack_top,
                           tak_bertanda64_t *heap_base)
{
    if (code_base != NULL) {
        *code_base = USER_CODE_BASE;
    }
    if (stack_top != NULL) {
        *stack_top = USER_STACK_TOP;
    }
    if (heap_base != NULL) {
        *heap_base = USER_HEAP_BASE;
    }
}
