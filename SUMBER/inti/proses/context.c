/*
 * PIGURA OS - CONTEXT.C
 * ----------------------
 * Implementasi context switching antar proses dan thread.
 *
 * Berkas ini berisi fungsi-fungsi untuk menyimpan dan memulihkan
 * konteks CPU saat terjadi context switch antar proses atau thread
 * dengan dukungan multi-arsitektur (x86, x86_64, ARM, ARM64).
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
#include "proses.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* Ukuran context structure */
#if defined(PIGURA_ARSITEKTUR_64BIT)
#define CONTEXT_SIZE            sizeof(cpu_context_t)
#else
#define CONTEXT_SIZE            sizeof(cpu_context_t)
#endif

/* Flag EFLAGS */
#define EFLAGS_IF               0x00000200
#define EFLAGS_IOPL_MASK        0x00003000
#define EFLAGS_IOPL_SHIFT       12
#define EFLAGS_NT               0x00004000
#define EFLAGS_RF               0x00010000
#define EFLAGS_VM               0x00020000

/* EFLAGS default */
#define EFLAGS_KERNEL_DEFAULT   (EFLAGS_IF)
#define EFLAGS_USER_DEFAULT     (EFLAGS_IF | (0 << EFLAGS_IOPL_SHIFT))

/* Segment selector untuk x86/x86_64 */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
#define SELECTOR_KERNEL_CODE    0x08
#define SELECTOR_KERNEL_DATA    0x10
#define SELECTOR_USER_CODE      0x18
#define SELECTOR_USER_DATA      0x20
#endif

/* CPSR bits untuk ARM */
#if defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7) || \
    defined(ARSITEKTUR_ARM64)
#define CPSR_MODE_USR           0x10
#define CPSR_MODE_SVC           0x13
#define CPSR_MODE_MASK          0x1F
#define CPSR_IRQ_DISABLE        0x80
#define CPSR_FIQ_DISABLE        0x40
#endif

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik context switch */
static struct {
    tak_bertanda64_t total_switches;
    tak_bertanda64_t kernel_to_user;
    tak_bertanda64_t user_to_kernel;
    tak_bertanda64_t user_to_user;
    tak_bertanda64_t contexts_created;
    tak_bertanda64_t contexts_destroyed;
    tak_bertanda64_t save_count;
    tak_bertanda64_t restore_count;
} context_stats = {0, 0, 0, 0, 0, 0, 0, 0};

/* Status inisialisasi */
static bool_t context_initialized = SALAH;

/* Lock */
static spinlock_t context_lock;

/*
 * ============================================================================
 * FUNGSI INTERNAL X86 32-BIT (INTERNAL X86 32-BIT FUNCTIONS)
 * ============================================================================
 */

#if defined(ARSITEKTUR_X86)

/*
 * context_save_x86
 * ----------------
 * Simpan context CPU untuk x86 32-bit.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: 0 jika berhasil
 */
static tak_bertanda32_t context_save_x86(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return 1;
    }
    
    /* Simpan general purpose registers */
    __asm__ __volatile__(
        "movl %%eax, %0\n\t"
        "movl %%ebx, %1\n\t"
        "movl %%ecx, %2\n\t"
        "movl %%edx, %3\n\t"
        "movl %%esi, %4\n\t"
        "movl %%edi, %5\n\t"
        "movl %%ebp, %6\n\t"
        : "=m"(ctx->eax), "=m"(ctx->ebx), "=m"(ctx->ecx),
          "=m"(ctx->edx), "=m"(ctx->esi), "=m"(ctx->edi),
          "=m"(ctx->ebp)
    );
    
    /* Simpan ESP dengan cara khusus */
    __asm__ __volatile__(
        "movl %%esp, %0"
        : "=m"(ctx->esp)
    );
    
    /* Simpan segment registers */
    __asm__ __volatile__(
        "movw %%cs, %0\n\t"
        "movw %%ds, %1\n\t"
        "movw %%es, %2\n\t"
        "movw %%fs, %3\n\t"
        "movw %%gs, %4\n\t"
        "movw %%ss, %5\n\t"
        : "=m"(ctx->cs), "=m"(ctx->ds), "=m"(ctx->es),
          "=m"(ctx->fs), "=m"(ctx->gs), "=m"(ctx->ss)
    );
    
    /* Simpan EIP - harus menggunakan call/pop trick */
    __asm__ __volatile__(
        "call 1f\n\t"
        "1: popl %0"
        : "=m"(ctx->eip)
    );
    
    /* Simpan EFLAGS */
    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0"
        : "=m"(ctx->eflags)
    );
    
    /* Simpan CR3 */
    ctx->cr3 = cpu_read_cr3();
    
    context_stats.save_count++;
    
    return 0;
}

#endif /* ARSITEKTUR_X86 */

/*
 * ============================================================================
 * FUNGSI INTERNAL X86_64 (INTERNAL X86_64 FUNCTIONS)
 * ============================================================================
 */

#if defined(ARSITEKTUR_X86_64)

/*
 * context_save_x86_64
 * -------------------
 * Simpan context CPU untuk x86_64.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: 0 jika berhasil
 */
static tak_bertanda32_t context_save_x86_64(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return 1;
    }
    
    /* Simpan general purpose registers */
    __asm__ __volatile__(
        "movq %%rax, %0\n\t"
        "movq %%rbx, %1\n\t"
        "movq %%rcx, %2\n\t"
        "movq %%rdx, %3\n\t"
        "movq %%rsi, %4\n\t"
        "movq %%rdi, %5\n\t"
        "movq %%rbp, %6\n\t"
        "movq %%rsp, %7\n\t"
        "movq %%r8, %8\n\t"
        "movq %%r9, %9\n\t"
        "movq %%r10, %10\n\t"
        "movq %%r11, %11\n\t"
        "movq %%r12, %12\n\t"
        "movq %%r13, %13\n\t"
        "movq %%r14, %14\n\t"
        "movq %%r15, %15\n\t"
        : "=m"(ctx->rax), "=m"(ctx->rbx), "=m"(ctx->rcx),
          "=m"(ctx->rdx), "=m"(ctx->rsi), "=m"(ctx->rdi),
          "=m"(ctx->rbp), "=m"(ctx->rsp),
          "=m"(ctx->r8), "=m"(ctx->r9), "=m"(ctx->r10),
          "=m"(ctx->r11), "=m"(ctx->r12), "=m"(ctx->r13),
          "=m"(ctx->r14), "=m"(ctx->r15)
    );
    
    /* Simpan segment registers */
    __asm__ __volatile__(
        "movw %%cs, %0\n\t"
        "movw %%ds, %1\n\t"
        "movw %%es, %2\n\t"
        "movw %%fs, %3\n\t"
        "movw %%gs, %4\n\t"
        "movw %%ss, %5\n\t"
        : "=m"(ctx->cs), "=m"(ctx->ds), "=m"(ctx->es),
          "=m"(ctx->fs), "=m"(ctx->gs), "=m"(ctx->ss)
    );
    
    /* Simpan RIP */
    __asm__ __volatile__(
        "leaq 0(%%rip), %0"
        : "=r"(ctx->rip)
    );
    
    /* Simpan RFLAGS */
    __asm__ __volatile__(
        "pushfq\n\t"
        "popq %0"
        : "=m"(ctx->rflags)
    );
    
    /* Simpan CR3 */
    ctx->cr3 = cpu_read_cr3();
    
    context_stats.save_count++;
    
    return 0;
}

#endif /* ARSITEKTUR_X86_64 */

/*
 * ============================================================================
 * FUNGSI RESTORE X86 32-BIT (RESTORE X86 32-BIT FUNCTIONS)
 * ============================================================================
 */

#if defined(ARSITEKTUR_X86)

/*
 * context_restore_x86
 * -------------------
 * Pulihkan context CPU untuk x86 32-bit.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
static void context_restore_x86(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    /* Pulihkan CR3 jika berbeda */
    if (ctx->cr3 != 0 && ctx->cr3 != cpu_read_cr3()) {
        cpu_write_cr3(ctx->cr3);
    }
    
    /* Pulihkan segment registers */
    __asm__ __volatile__(
        "movw %0, %%ds\n\t"
        "movw %1, %%es\n\t"
        "movw %2, %%fs\n\t"
        "movw %3, %%gs\n\t"
        :
        : "r"(ctx->ds), "r"(ctx->es), "r"(ctx->fs), "r"(ctx->gs)
        : "memory"
    );
    
    /* Pulihkan general purpose registers */
    __asm__ __volatile__(
        "movl %0, %%eax\n\t"
        "movl %1, %%ebx\n\t"
        "movl %2, %%ecx\n\t"
        "movl %3, %%edx\n\t"
        "movl %4, %%esi\n\t"
        "movl %5, %%edi\n\t"
        "movl %6, %%ebp\n\t"
        "movl %7, %%esp\n\t"
        :
        : "m"(ctx->eax), "m"(ctx->ebx), "m"(ctx->ecx),
          "m"(ctx->edx), "m"(ctx->esi), "m"(ctx->edi),
          "m"(ctx->ebp), "m"(ctx->esp)
        : "memory"
    );
    
    /* Pulihkan EFLAGS */
    __asm__ __volatile__(
        "pushl %0\n\t"
        "popfl\n\t"
        :
        : "m"(ctx->eflags)
        : "memory", "cc"
    );
    
    context_stats.restore_count++;
    
    /* Jump ke EIP */
    __asm__ __volatile__(
        "jmp *%0"
        :
        : "r"(ctx->eip)
        : "memory"
    );
}

#endif /* ARSITEKTUR_X86 */

/*
 * ============================================================================
 * FUNGSI RESTORE X86_64 (RESTORE X86_64 FUNCTIONS)
 * ============================================================================
 */

#if defined(ARSITEKTUR_X86_64)

/*
 * context_restore_x86_64
 * ----------------------
 * Pulihkan context CPU untuk x86_64.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
static void context_restore_x86_64(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    /* Pulihkan CR3 jika berbeda */
    if (ctx->cr3 != 0 && ctx->cr3 != cpu_read_cr3()) {
        cpu_write_cr3(ctx->cr3);
    }
    
    /* Pulihkan segment registers */
    __asm__ __volatile__(
        "movw %0, %%ds\n\t"
        "movw %1, %%es\n\t"
        "movw %2, %%fs\n\t"
        "movw %3, %%gs\n\t"
        :
        : "r"(ctx->ds), "r"(ctx->es), "r"(ctx->fs), "r"(ctx->gs)
        : "memory"
    );
    
    /* Pulihkan general purpose registers */
    __asm__ __volatile__(
        "movq %0, %%rax\n\t"
        "movq %1, %%rbx\n\t"
        "movq %2, %%rcx\n\t"
        "movq %3, %%rdx\n\t"
        "movq %4, %%rsi\n\t"
        "movq %5, %%rdi\n\t"
        "movq %6, %%rbp\n\t"
        "movq %7, %%rsp\n\t"
        "movq %8, %%r8\n\t"
        "movq %9, %%r9\n\t"
        "movq %10, %%r10\n\t"
        "movq %11, %%r11\n\t"
        "movq %12, %%r12\n\t"
        "movq %13, %%r13\n\t"
        "movq %14, %%r14\n\t"
        "movq %15, %%r15\n\t"
        :
        : "m"(ctx->rax), "m"(ctx->rbx), "m"(ctx->rcx),
          "m"(ctx->rdx), "m"(ctx->rsi), "m"(ctx->rdi),
          "m"(ctx->rbp), "m"(ctx->rsp),
          "m"(ctx->r8), "m"(ctx->r9), "m"(ctx->r10),
          "m"(ctx->r11), "m"(ctx->r12), "m"(ctx->r13),
          "m"(ctx->r14), "m"(ctx->r15)
        : "memory"
    );
    
    /* Pulihkan RFLAGS */
    __asm__ __volatile__(
        "pushq %0\n\t"
        "popfq\n\t"
        :
        : "m"(ctx->rflags)
        : "memory", "cc"
    );
    
    context_stats.restore_count++;
    
    /* Jump ke RIP */
    __asm__ __volatile__(
        "jmp *%0"
        :
        : "r"(ctx->rip)
        : "memory"
    );
}

#endif /* ARSITEKTUR_X86_64 */

/*
 * ============================================================================
 * FUNGSI INTERNAL ARM (INTERNAL ARM FUNCTIONS)
 * ============================================================================
 */

#if defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)

/*
 * context_save_arm
 * ----------------
 * Simpan context CPU untuk ARM 32-bit.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: 0 jika berhasil
 */
static tak_bertanda32_t context_save_arm(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return 1;
    }
    
    /* Simpan general purpose registers r0-r12 */
    __asm__ __volatile__(
        "stmia %0, {r0-r12}\n\t"
        :
        : "r"(&ctx->eax)  /* r0-r12 disimpan mulai dari eax */
        : "memory"
    );
    
    /* Simpan SP, LR, PC */
    __asm__ __volatile__(
        "mov %0, sp\n\t"
        "mov %1, lr\n\t"
        "mov %2, pc\n\t"
        : "=r"(ctx->esp), "=r"(ctx->lr_usr), "=r"(ctx->eip)
    );
    
    /* Simpan CPSR */
    __asm__ __volatile__(
        "mrs %0, cpsr"
        : "=r"(ctx->cpsr)
    );
    
    /* Simpan SPSR jika di mode SVC */
    __asm__ __volatile__(
        "mrs %0, spsr"
        : "=r"(ctx->spsr)
    );
    
    context_stats.save_count++;
    
    return 0;
}

/*
 * context_restore_arm
 * -------------------
 * Pulihkan context CPU untuk ARM 32-bit.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
static void context_restore_arm(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    /* Pulihkan general purpose registers r0-r12 */
    __asm__ __volatile__(
        "ldmia %0, {r0-r12}\n\t"
        :
        : "r"(&ctx->eax)
        : "memory"
    );
    
    /* Pulihkan SP, LR */
    __asm__ __volatile__(
        "mov sp, %0\n\t"
        "mov lr, %1\n\t"
        :
        : "r"(ctx->esp), "r"(ctx->lr_usr)
        : "memory"
    );
    
    context_stats.restore_count++;
    
    /* Return ke PC dengan SPSR */
    __asm__ __volatile__(
        "msr spsr_cxsf, %0\n\t"
        "movs pc, %1"
        :
        : "r"(ctx->spsr), "r"(ctx->eip)
        : "memory"
    );
}

#endif /* ARSITEKTUR_ARM || ARSITEKTUR_ARMV7 */

#if defined(ARSITEKTUR_ARM64)

/*
 * context_save_arm64
 * ------------------
 * Simpan context CPU untuk ARM64.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: 0 jika berhasil
 */
static tak_bertanda32_t context_save_arm64(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return 1;
    }
    
    /* Simpan general purpose registers x0-x30, sp */
    __asm__ __volatile__(
        "stp x0, x1, [%0, #0]\n\t"
        "stp x2, x3, [%0, #16]\n\t"
        "stp x4, x5, [%0, #32]\n\t"
        "stp x6, x7, [%0, #48]\n\t"
        "stp x8, x9, [%0, #64]\n\t"
        "stp x10, x11, [%0, #80]\n\t"
        "stp x12, x13, [%0, #96]\n\t"
        "stp x14, x15, [%0, #112]\n\t"
        "stp x16, x17, [%0, #128]\n\t"
        "stp x18, x19, [%0, #144]\n\t"
        "stp x20, x21, [%0, #160]\n\t"
        "stp x22, x23, [%0, #176]\n\t"
        "stp x24, x25, [%0, #192]\n\t"
        "stp x26, x27, [%0, #208]\n\t"
        "stp x28, x29, [%0, #224]\n\t"
        "mov %1, sp\n\t"
        "mov %2, x30\n\t"
        :
        : "r"(ctx), "r"(ctx->rsp), "r"(ctx->lr)
        : "memory"
    );
    
    /* Simpan ELR_EL1 dan SPSR_EL1 */
    __asm__ __volatile__(
        "mrs %0, elr_el1\n\t"
        "mrs %1, spsr_el1\n\t"
        : "=r"(ctx->elr_el1), "=r"(ctx->spsr_el1)
    );
    
    /* Simpan TPIDR_EL0 */
    __asm__ __volatile__(
        "mrs %0, tpidr_el0"
        : "=r"(ctx->tpidr_el0)
    );
    
    context_stats.save_count++;
    
    return 0;
}

/*
 * context_restore_arm64
 * ---------------------
 * Pulihkan context CPU untuk ARM64.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
static void context_restore_arm64(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    /* Pulihkan general purpose registers */
    __asm__ __volatile__(
        "ldp x0, x1, [%0, #0]\n\t"
        "ldp x2, x3, [%0, #16]\n\t"
        "ldp x4, x5, [%0, #32]\n\t"
        "ldp x6, x7, [%0, #48]\n\t"
        "ldp x8, x9, [%0, #64]\n\t"
        "ldp x10, x11, [%0, #80]\n\t"
        "ldp x12, x13, [%0, #96]\n\t"
        "ldp x14, x15, [%0, #112]\n\t"
        "ldp x16, x17, [%0, #128]\n\t"
        "ldp x18, x19, [%0, #144]\n\t"
        "ldp x20, x21, [%0, #160]\n\t"
        "ldp x22, x23, [%0, #176]\n\t"
        "ldp x24, x25, [%0, #192]\n\t"
        "ldp x26, x27, [%0, #208]\n\t"
        "ldp x28, x29, [%0, #224]\n\t"
        "mov sp, %1\n\t"
        :
        : "r"(ctx), "r"(ctx->rsp)
        : "memory"
    );
    
    /* Pulihkan TPIDR_EL0 */
    __asm__ __volatile__(
        "msr tpidr_el0, %0"
        :
        : "r"(ctx->tpidr_el0)
        : "memory"
    );
    
    /* Pulihkan ELR_EL1 dan SPSR_EL1 */
    __asm__ __volatile__(
        "msr elr_el1, %0\n\t"
        "msr spsr_el1, %1"
        :
        : "r"(ctx->elr_el1), "r"(ctx->spsr_el1)
        : "memory"
    );
    
    context_stats.restore_count++;
    
    /* Eret ke EL0 */
    __asm__ __volatile__(
        "eret"
    );
}

#endif /* ARSITEKTUR_ARM64 */

/*
 * ============================================================================
 * FUNGSI ALOKASI CONTEXT (CONTEXT ALLOCATION FUNCTIONS)
 * ============================================================================
 */

/*
 * context_alloc
 * -------------
 * Alokasikan struktur context baru.
 *
 * Return: Pointer ke context, atau NULL jika gagal
 */
static cpu_context_t *context_alloc(void)
{
    cpu_context_t *ctx;
    
    ctx = (cpu_context_t *)kmalloc(sizeof(cpu_context_t));
    
    if (ctx == NULL) {
        return NULL;
    }
    
    kernel_memset(ctx, 0, sizeof(cpu_context_t));
    
    context_stats.contexts_created++;
    
    return ctx;
}

/*
 * context_free
 * ------------
 * Bebaskan struktur context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
static void context_free(cpu_context_t *ctx)
{
    if (ctx != NULL) {
        kfree(ctx);
        context_stats.contexts_destroyed++;
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIK CONTEXT (PUBLIC CONTEXT FUNCTIONS)
 * ============================================================================
 */

/*
 * context_init
 * ------------
 * Inisialisasi subsistem context.
 *
 * Return: Status operasi
 */
status_t context_init(void)
{
    if (context_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Reset statistik */
    kernel_memset(&context_stats, 0, sizeof(context_stats));
    
    /* Init lock */
    spinlock_init(&context_lock);
    
    context_initialized = BENAR;
    
    kernel_printf("[CONTEXT] Context manager initialized\n");
    
    return STATUS_BERHASIL;
}

/*
 * context_buat
 * ------------
 * Buat context baru untuk proses/thread.
 *
 * Parameter:
 *   entry_point - Alamat entry point
 *   stack_ptr   - Pointer ke stack
 *   is_user     - Flag user mode
 *
 * Return: Pointer ke context, atau NULL jika gagal
 */
cpu_context_t *context_buat(alamat_virtual_t entry_point, void *stack_ptr,
                            bool_t is_user)
{
    cpu_context_t *ctx;
    
    ctx = context_alloc();
    if (ctx == NULL) {
        return NULL;
    }
    
    spinlock_kunci(&context_lock);
    
#if defined(ARSITEKTUR_X86)
    /* Setup context untuk x86 32-bit */
    ctx->eip = (tanda32_t)entry_point;
    ctx->esp = (tanda32_t)(alamat_ptr_t)stack_ptr;
    ctx->ebp = 0;
    
    if (is_user) {
        ctx->cs = SELECTOR_USER_CODE | 0x03;
        ctx->ds = SELECTOR_USER_DATA | 0x03;
        ctx->es = SELECTOR_USER_DATA | 0x03;
        ctx->fs = SELECTOR_USER_DATA | 0x03;
        ctx->gs = SELECTOR_USER_DATA | 0x03;
        ctx->ss = SELECTOR_USER_DATA | 0x03;
        ctx->eflags = EFLAGS_USER_DEFAULT;
    } else {
        ctx->cs = SELECTOR_KERNEL_CODE;
        ctx->ds = SELECTOR_KERNEL_DATA;
        ctx->es = SELECTOR_KERNEL_DATA;
        ctx->fs = SELECTOR_KERNEL_DATA;
        ctx->gs = SELECTOR_KERNEL_DATA;
        ctx->ss = SELECTOR_KERNEL_DATA;
        ctx->eflags = EFLAGS_KERNEL_DEFAULT;
    }
    
    ctx->cr3 = 0;
    
#elif defined(ARSITEKTUR_X86_64)
    /* Setup context untuk x86_64 */
    ctx->rip = (tanda64_t)entry_point;
    ctx->rsp = (tanda64_t)(alamat_ptr_t)stack_ptr;
    ctx->rbp = 0;
    
    if (is_user) {
        ctx->cs = SELECTOR_USER_CODE | 0x03;
        ctx->ds = SELECTOR_USER_DATA | 0x03;
        ctx->es = SELECTOR_USER_DATA | 0x03;
        ctx->fs = SELECTOR_USER_DATA | 0x03;
        ctx->gs = SELECTOR_USER_DATA | 0x03;
        ctx->ss = SELECTOR_USER_DATA | 0x03;
        ctx->rflags = EFLAGS_USER_DEFAULT;
    } else {
        ctx->cs = SELECTOR_KERNEL_CODE;
        ctx->ds = SELECTOR_KERNEL_DATA;
        ctx->es = SELECTOR_KERNEL_DATA;
        ctx->fs = SELECTOR_KERNEL_DATA;
        ctx->gs = SELECTOR_KERNEL_DATA;
        ctx->ss = SELECTOR_KERNEL_DATA;
        ctx->rflags = EFLAGS_KERNEL_DEFAULT;
    }
    
    ctx->cr3 = 0;
    
#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)
    /* Setup context untuk ARM 32-bit */
    ctx->eip = (tanda32_t)entry_point;
    ctx->esp = (tanda32_t)(alamat_ptr_t)stack_ptr;
    ctx->ebp = 0;
    ctx->lr_usr = 0;
    
    if (is_user) {
        ctx->cpsr = CPSR_MODE_USR;
        ctx->spsr = CPSR_MODE_USR;
    } else {
        ctx->cpsr = CPSR_MODE_SVC | CPSR_IRQ_DISABLE;
        ctx->spsr = CPSR_MODE_SVC | CPSR_IRQ_DISABLE;
    }
    
#elif defined(ARSITEKTUR_ARM64)
    /* Setup context untuk ARM64 */
    ctx->elr_el1 = (tanda64_t)entry_point;
    ctx->rsp = (tanda64_t)(alamat_ptr_t)stack_ptr;
    ctx->rbp = 0;
    ctx->lr = 0;
    
    if (is_user) {
        ctx->spsr_el1 = 0x00000000;  /* EL0t */
    } else {
        ctx->spsr_el1 = 0x00000100;  /* EL1h */
    }
    
#endif
    
    spinlock_buka(&context_lock);
    
    return ctx;
}

/*
 * context_hancurkan
 * -----------------
 * Hancurkan context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
void context_hancurkan(cpu_context_t *ctx)
{
    context_free(ctx);
}

/*
 * context_switch
 * --------------
 * Lakukan context switch dari proses lama ke proses baru.
 *
 * Parameter:
 *   old_proses - Pointer ke proses lama (boleh NULL)
 *   new_proses - Pointer ke proses baru
 */
void context_switch(proses_t *old_proses, proses_t *new_proses)
{
    cpu_context_t *old_ctx;
    cpu_context_t *new_ctx;
    
    if (!context_initialized || new_proses == NULL) {
        return;
    }
    
    if (new_proses->main_thread == NULL) {
        return;
    }
    
    new_ctx = (cpu_context_t *)new_proses->main_thread->context;
    if (new_ctx == NULL) {
        return;
    }
    
    /* Update statistik */
    context_stats.total_switches++;
    
    if (old_proses != NULL) {
        if ((old_proses->flags & PROSES_FLAG_KERNEL) &&
            !(new_proses->flags & PROSES_FLAG_KERNEL)) {
            context_stats.kernel_to_user++;
        } else if (!(old_proses->flags & PROSES_FLAG_KERNEL) &&
                   (new_proses->flags & PROSES_FLAG_KERNEL)) {
            context_stats.user_to_kernel++;
        } else if (!(old_proses->flags & PROSES_FLAG_KERNEL) &&
                   !(new_proses->flags & PROSES_FLAG_KERNEL)) {
            context_stats.user_to_user++;
        }
    }
    
    /* Simpan context lama */
    if (old_proses != NULL && old_proses->main_thread != NULL &&
        old_proses->main_thread->context != NULL) {
        old_ctx = (cpu_context_t *)old_proses->main_thread->context;
    } else {
        old_ctx = NULL;
    }
    
    /* Set proses saat ini */
    proses_set_saat_ini(new_proses);
    
    /* Switch CR3 jika berbeda address space */
    if (new_proses->vm != NULL && new_ctx->cr3 != 0) {
        if (new_ctx->cr3 != cpu_read_cr3()) {
            cpu_write_cr3(new_ctx->cr3);
        }
    }
    
    /* Lakukan context switch ke new context */
    context_switch_to(new_ctx);
}

/*
 * context_switch_thread
 * ---------------------
 * Lakukan context switch antara dua thread.
 *
 * Parameter:
 *   old_thread - Pointer ke thread lama (boleh NULL)
 *   new_thread - Pointer ke thread baru
 */
void context_switch_thread(thread_t *old_thread, thread_t *new_thread)
{
    cpu_context_t *old_ctx;
    cpu_context_t *new_ctx;
    
    if (!context_initialized || new_thread == NULL) {
        return;
    }
    
    new_ctx = (cpu_context_t *)new_thread->context;
    if (new_ctx == NULL) {
        return;
    }
    
    context_stats.total_switches++;
    
    /* Simpan context lama */
    if (old_thread != NULL && old_thread->context != NULL) {
        old_ctx = (cpu_context_t *)old_thread->context;
    } else {
        old_ctx = NULL;
    }
    
    /* Lakukan context switch */
    context_switch_to(new_ctx);
}

/*
 * context_switch_to
 * -----------------
 * Switch ke context tertentu tanpa menyimpan context lama.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
void context_switch_to(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
#if defined(ARSITEKTUR_X86)
    context_restore_x86(ctx);
#elif defined(ARSITEKTUR_X86_64)
    context_restore_x86_64(ctx);
#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)
    context_restore_arm(ctx);
#elif defined(ARSITEKTUR_ARM64)
    context_restore_arm64(ctx);
#endif
}

/*
 * context_set_entry
 * -----------------
 * Set entry point context.
 *
 * Parameter:
 *   ctx         - Pointer ke context
 *   entry_point - Alamat entry point
 *
 * Return: Status operasi
 */
status_t context_set_entry(cpu_context_t *ctx, alamat_virtual_t entry_point)
{
    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
#if defined(PIGURA_ARSITEKTUR_64BIT)
    ctx->rip = (tanda64_t)entry_point;
#else
    ctx->eip = (tanda32_t)entry_point;
#endif
    
    return STATUS_BERHASIL;
}

/*
 * context_set_stack
 * -----------------
 * Set stack pointer context.
 *
 * Parameter:
 *   ctx        - Pointer ke context
 *   stack_ptr  - Pointer ke stack
 *
 * Return: Status operasi
 */
status_t context_set_stack(cpu_context_t *ctx, void *stack_ptr)
{
    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
#if defined(PIGURA_ARSITEKTUR_64BIT)
    ctx->rsp = (tanda64_t)(alamat_ptr_t)stack_ptr;
#else
    ctx->esp = (tanda32_t)(alamat_ptr_t)stack_ptr;
#endif
    
    return STATUS_BERHASIL;
}

/*
 * context_set_cr3
 * ---------------
 * Set CR3 (page directory) context.
 *
 * Parameter:
 *   ctx  - Pointer ke context
 *   cr3  - Nilai CR3
 *
 * Return: Status operasi
 */
status_t context_set_cr3(cpu_context_t *ctx, tak_bertanda64_t cr3)
{
    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    ctx->cr3 = cr3;
    
    return STATUS_BERHASIL;
}

/*
 * context_set_return
 * ------------------
 * Set return value dalam context.
 *
 * Parameter:
 *   ctx    - Pointer ke context
 *   retval - Return value
 */
void context_set_return(cpu_context_t *ctx, tanda64_t retval)
{
    if (ctx == NULL) {
        return;
    }
    
#if defined(PIGURA_ARSITEKTUR_64BIT)
    ctx->rax = (tanda64_t)retval;
#else
    ctx->eax = (tanda32_t)retval;
#endif
}

/*
 * context_dapat_stack
 * -------------------
 * Dapatkan stack pointer dari context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: Stack pointer
 */
void *context_dapat_stack(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return NULL;
    }
    
#if defined(PIGURA_ARSITEKTUR_64BIT)
    return (void *)(alamat_ptr_t)ctx->rsp;
#else
    return (void *)(alamat_ptr_t)ctx->esp;
#endif
}

/*
 * context_dapat_entry
 * -------------------
 * Dapatkan entry point dari context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: Entry point
 */
alamat_virtual_t context_dapat_entry(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return 0;
    }
    
#if defined(PIGURA_ARSITEKTUR_64BIT)
    return (alamat_virtual_t)ctx->rip;
#else
    return (alamat_virtual_t)ctx->eip;
#endif
}

/*
 * context_dapat_cr3
 * -----------------
 * Dapatkan CR3 dari context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: Nilai CR3
 */
tak_bertanda64_t context_dapat_cr3(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return 0;
    }
    
    return ctx->cr3;
}

/*
 * context_setup_user
 * ------------------
 * Setup context untuk user mode.
 *
 * Parameter:
 *   ctx        - Pointer ke context
 *   entry      - Entry point
 *   stack_top  - Alamat atas stack
 *   argc       - Jumlah argumen
 *   argv       - Pointer ke array argumen
 *
 * Return: Status operasi
 */
status_t context_setup_user(cpu_context_t *ctx, alamat_virtual_t entry,
                            alamat_virtual_t stack_top,
                            tak_bertanda32_t argc, tak_bertanda32_t argv)
{
    tak_bertanda32_t *stack;
    
    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Setup stack dengan argumen */
    stack = (tak_bertanda32_t *)stack_top;
    
    /* Push argv dan argc */
    stack--;
    *stack = argv;
    stack--;
    *stack = argc;
    
    /* Set context */
    context_set_entry(ctx, entry);
    context_set_stack(ctx, stack);
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    ctx->cs = SELECTOR_USER_CODE | 0x03;
    ctx->ds = SELECTOR_USER_DATA | 0x03;
    ctx->es = SELECTOR_USER_DATA | 0x03;
    ctx->fs = SELECTOR_USER_DATA | 0x03;
    ctx->gs = SELECTOR_USER_DATA | 0x03;
    ctx->ss = SELECTOR_USER_DATA | 0x03;
    
#if defined(PIGURA_ARSITEKTUR_64BIT)
    ctx->rflags = EFLAGS_USER_DEFAULT;
#else
    ctx->eflags = EFLAGS_USER_DEFAULT;
#endif
    
#endif
    
    return STATUS_BERHASIL;
}

/*
 * context_setup_kernel
 * --------------------
 * Setup context untuk kernel thread.
 *
 * Parameter:
 *   ctx   - Pointer ke context
 *   entry - Entry point
 *   arg   - Argumen untuk thread
 *
 * Return: Status operasi
 */
status_t context_setup_kernel(cpu_context_t *ctx, alamat_virtual_t entry,
                              void *arg)
{
    if (ctx == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    context_set_entry(ctx, entry);
    
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
    ctx->cs = SELECTOR_KERNEL_CODE;
    ctx->ds = SELECTOR_KERNEL_DATA;
    ctx->es = SELECTOR_KERNEL_DATA;
    ctx->fs = SELECTOR_KERNEL_DATA;
    ctx->gs = SELECTOR_KERNEL_DATA;
    ctx->ss = SELECTOR_KERNEL_DATA;
#endif
    
#if defined(PIGURA_ARSITEKTUR_64BIT)
    ctx->rflags = EFLAGS_KERNEL_DEFAULT;
#else
    ctx->eflags = EFLAGS_KERNEL_DEFAULT;
#endif
    
    /* Push argumen ke stack */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    if (ctx->rsp != 0) {
        tak_bertanda64_t *sp = (tak_bertanda64_t *)ctx->rsp;
        sp--;
        *sp = (tak_bertanda64_t)(alamat_ptr_t)arg;
        ctx->rsp = (tak_bertanda64_t)(alamat_ptr_t)sp;
    }
#else
    if (ctx->esp != 0) {
        tak_bertanda32_t *sp = (tak_bertanda32_t *)ctx->esp;
        sp--;
        *sp = (tak_bertanda32_t)(alamat_ptr_t)arg;
        ctx->esp = (tak_bertanda32_t)(alamat_ptr_t)sp;
    }
#endif
    
    return STATUS_BERHASIL;
}

/*
 * context_dup
 * -----------
 * Duplikasi context untuk fork.
 *
 * Parameter:
 *   ctx - Pointer ke context sumber
 *
 * Return: Pointer ke context baru, atau NULL
 */
cpu_context_t *context_dup(cpu_context_t *ctx)
{
    cpu_context_t *new_ctx;
    
    if (ctx == NULL) {
        return NULL;
    }
    
    new_ctx = context_alloc();
    if (new_ctx == NULL) {
        return NULL;
    }
    
    /* Copy semua field */
    kernel_memcpy(new_ctx, ctx, sizeof(cpu_context_t));
    
    /* Set return value 0 untuk child */
    context_set_return(new_ctx, 0);
    
    return new_ctx;
}

/*
 * context_return_to_user
 * ----------------------
 * Return ke user mode dari system call.
 *
 * Parameter:
 *   ctx - Pointer ke context user
 */
void context_return_to_user(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        PANIC("context_return_to_user: context NULL");
    }
    
    context_stats.user_to_kernel++;
    
#if defined(ARSITEKTUR_X86)
    /* Restore ke user mode menggunakan IRET */
    __asm__ __volatile__(
        "movw %0, %%ds\n\t"
        "movw %0, %%es\n\t"
        "movw %0, %%fs\n\t"
        "movw %0, %%gs\n\t"
        "pushl %1\n\t"       /* SS */
        "pushl %2\n\t"       /* ESP */
        "pushl %3\n\t"       /* EFLAGS */
        "pushl %4\n\t"       /* CS */
        "pushl %5\n\t"       /* EIP */
        "movl %6, %%eax\n\t"
        "movl %7, %%ebx\n\t"
        "movl %8, %%ecx\n\t"
        "movl %9, %%edx\n\t"
        "movl %10, %%esi\n\t"
        "movl %11, %%edi\n\t"
        "movl %12, %%ebp\n\t"
        "iret\n\t"
        :
        : "r"((tak_bertanda32_t)ctx->ss),
          "r"((tak_bertanda32_t)ctx->ss),
          "r"(ctx->esp),
          "r"(ctx->eflags),
          "r"((tak_bertanda32_t)ctx->cs),
          "r"(ctx->eip),
          "m"(ctx->eax),
          "m"(ctx->ebx),
          "m"(ctx->ecx),
          "m"(ctx->edx),
          "m"(ctx->esi),
          "m"(ctx->edi),
          "m"(ctx->ebp)
        : "memory"
    );
    
#elif defined(ARSITEKTUR_X86_64)
    /* Restore ke user mode menggunakan IRETQ */
    __asm__ __volatile__(
        "movw %0, %%ds\n\t"
        "movw %0, %%es\n\t"
        "movw %0, %%fs\n\t"
        "movw %0, %%gs\n\t"
        "swapgs\n\t"
        "pushq %1\n\t"       /* SS */
        "pushq %2\n\t"       /* RSP */
        "pushq %3\n\t"       /* RFLAGS */
        "pushq %4\n\t"       /* CS */
        "pushq %5\n\t"       /* RIP */
        "movq %6, %%rax\n\t"
        "movq %7, %%rbx\n\t"
        "movq %8, %%rcx\n\t"
        "movq %9, %%rdx\n\t"
        "movq %10, %%rsi\n\t"
        "movq %11, %%rdi\n\t"
        "movq %12, %%rbp\n\t"
        "iretq\n\t"
        :
        : "r"((tak_bertanda16_t)ctx->ss),
          "r"((tak_bertanda64_t)ctx->ss),
          "r"(ctx->rsp),
          "r"(ctx->rflags),
          "r"((tak_bertanda64_t)ctx->cs),
          "r"(ctx->rip),
          "m"(ctx->rax),
          "m"(ctx->rbx),
          "m"(ctx->rcx),
          "m"(ctx->rdx),
          "m"(ctx->rsi),
          "m"(ctx->rdi),
          "m"(ctx->rbp)
        : "memory"
    );
    
#else
    /* Untuk ARM, gunakan context_switch_to */
    context_switch_to(ctx);
#endif
}

/*
 * context_valid
 * -------------
 * Validasi pointer context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 *
 * Return: BENAR jika valid
 */
bool_t context_valid(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return SALAH;
    }
    
    /* Cek entry point valid */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    if (ctx->rip == 0) {
        return SALAH;
    }
    if (ctx->rsp == 0) {
        return SALAH;
    }
#else
    if (ctx->eip == 0) {
        return SALAH;
    }
    if (ctx->esp == 0) {
        return SALAH;
    }
#endif
    
    return BENAR;
}

/*
 * context_dapat_statistik
 * -----------------------
 * Dapatkan statistik context switch.
 *
 * Parameter:
 *   total        - Pointer untuk total switches
 *   k_to_u       - Pointer untuk kernel to user switches
 *   u_to_k       - Pointer untuk user to kernel switches
 *   u_to_u       - Pointer untuk user to user switches
 */
void context_dapat_statistik(tak_bertanda64_t *total,
                             tak_bertanda64_t *k_to_u,
                             tak_bertanda64_t *u_to_k,
                             tak_bertanda64_t *u_to_u)
{
    if (total != NULL) {
        *total = context_stats.total_switches;
    }
    
    if (k_to_u != NULL) {
        *k_to_u = context_stats.kernel_to_user;
    }
    
    if (u_to_k != NULL) {
        *u_to_k = context_stats.user_to_kernel;
    }
    
    if (u_to_u != NULL) {
        *u_to_u = context_stats.user_to_user;
    }
}

/*
 * context_print_info
 * ------------------
 * Print informasi context.
 *
 * Parameter:
 *   ctx - Pointer ke context
 */
void context_print_info(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        kernel_printf("[CONTEXT] Context: NULL\n");
        return;
    }
    
    kernel_printf("\n[CONTEXT] Informasi Context:\n");
    kernel_printf("========================================\n");
    
#if defined(PIGURA_ARSITEKTUR_64BIT)
    kernel_printf("  RIP:    0x%016llX\n", ctx->rip);
    kernel_printf("  RSP:    0x%016llX\n", ctx->rsp);
    kernel_printf("  RBP:    0x%016llX\n", ctx->rbp);
    kernel_printf("  RFLAGS: 0x%016llX\n", ctx->rflags);
    kernel_printf("  CR3:    0x%016llX\n", ctx->cr3);
    kernel_printf("  CS:     0x%04X  DS: 0x%04X\n",
                  ctx->cs, ctx->ds);
    kernel_printf("  SS:     0x%04X\n", ctx->ss);
    kernel_printf("  RAX:    0x%016llX\n", ctx->rax);
    kernel_printf("  RBX:    0x%016llX\n", ctx->rbx);
    kernel_printf("  RCX:    0x%016llX\n", ctx->rcx);
    kernel_printf("  RDX:    0x%016llX\n", ctx->rdx);
#else
    kernel_printf("  EIP:    0x%08lX\n", ctx->eip);
    kernel_printf("  ESP:    0x%08lX\n", ctx->esp);
    kernel_printf("  EBP:    0x%08lX\n", ctx->ebp);
    kernel_printf("  EFLAGS: 0x%08lX\n", ctx->eflags);
    kernel_printf("  CR3:    0x%08lX\n", ctx->cr3);
    kernel_printf("  CS:     0x%04X  DS: 0x%04X\n",
                  ctx->cs, ctx->ds);
    kernel_printf("  SS:     0x%04X\n", ctx->ss);
    kernel_printf("  EAX:    0x%08lX\n", ctx->eax);
    kernel_printf("  EBX:    0x%08lX\n", ctx->ebx);
    kernel_printf("  ECX:    0x%08lX\n", ctx->ecx);
    kernel_printf("  EDX:    0x%08lX\n", ctx->edx);
#endif
    
    kernel_printf("========================================\n");
}
