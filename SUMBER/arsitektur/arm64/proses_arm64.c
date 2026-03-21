/*
 * PIGURA OS - PROSES_ARM64.C
 * --------------------------
 * Implementasi manajemen proses untuk arsitektur ARM64 (AArch64).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola proses dan thread
 * pada prosesor ARM64, termasuk context switching dan scheduler.
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

/* Stack size */
#define UKURAN_STACK_KERNEL     (8 * 1024)      /* 8KB kernel stack */
#define UKURAN_STACK_USER       (256 * 1024)    /* 256KB user stack */

/* Process flags */
#define PROSES_FLAG_KERNEL      0x0001
#define PROSES_FLAG_RUNNING     0x0002
#define PROSES_FLAG_READY       0x0004

/* SPSR bits */
#define SPSR_EL0t               0x00000000
#define SPSR_EL1t               0x00000004
#define SPSR_EL1h               0x00000005
#define SPSR_EL2t               0x00000008
#define SPSR_EL2h               0x00000009
#define SPSR_EL3t               0x0000000C
#define SPSR_EL3h               0x0000000D

/* SPSR mask bits */
#define SPSR_D                  (1 << 9)
#define SPSR_A                  (1 << 8)
#define SPSR_I                  (1 << 7)
#define SPSR_F                  (1 << 6)

/* TPIDR_EL0 register for TLS */
#define TPIDR_EL0               "tpidr_el0"

/* Maximum processes */
#define PROSES_MAX              256

/* Maximum threads per process */
#define THREAD_MAX              64

/*
 * ============================================================================
 * TIPE DATA LOKAL (LOCAL DATA TYPES)
 * ============================================================================
 */

/* CPU context untuk ARM64 */
typedef struct {
    tak_bertanda64_t x19;       /* Callee-saved registers */
    tak_bertanda64_t x20;
    tak_bertanda64_t x21;
    tak_bertanda64_t x22;
    tak_bertanda64_t x23;
    tak_bertanda64_t x24;
    tak_bertanda64_t x25;
    tak_bertanda64_t x26;
    tak_bertanda64_t x27;
    tak_bertanda64_t x28;
    tak_bertanda64_t x29;       /* Frame pointer */
    tak_bertanda64_t x30;       /* Link register */
    tak_bertanda64_t sp;        /* Stack pointer */
    tak_bertanda64_t pc;        /* Program counter */
    tak_bertanda64_t spsr;      /* Saved processor state */
    tak_bertanda64_t tpidr;     /* Thread pointer */
} cpu_context_t;

/* Thread control block */
typedef struct thread {
    tid_t tid;
    pid_t pid;
    cpu_context_t context;
    void *stack;
    ukuran_t stack_size;
    tak_bertanda32_t flags;
    proses_status_t status;
    prioritas_t prioritas;
    tak_bertanda64_t waktu_cpu;
    struct thread *next;
    struct thread *prev;
} thread_t;

/* Process control block */
typedef struct proses {
    pid_t pid;
    pid_t ppid;
    thread_t *threads[THREAD_MAX];
    tak_bertanda32_t thread_count;
    tak_bertanda64_t pgd;       /* Page table */
    tak_bertanda32_t asid;
    tak_bertanda32_t flags;
    proses_status_t status;
    prioritas_t prioritas;
    tak_bertanda64_t mem_used;
    char nama[32];
} proses_t;

/*
 * ============================================================================
 * VARIABEL GLOBAL (GLOBAL VARIABLES)
 * ============================================================================
 */

/* Process table */
static proses_t g_proses_table[PROSES_MAX];

/* Thread table */
static thread_t g_thread_table[PROSES_MAX * THREAD_MAX];

/* Current process */
static proses_t *g_proses_current = NULL;

/* Current thread */
static thread_t *g_thread_current = NULL;

/* Scheduler queues */
static thread_t *g_ready_queue[PRIORITAS_REALTIME + 1];

/* PID counter */
static pid_t g_pid_counter = 1;

/* TID counter */
static tid_t g_tid_counter = 1;

/* Flag inisialisasi */
static bool_t g_proses_diinisalisasi = SALAH;

/* Idle thread */
static thread_t g_idle_thread;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * _alloc_pid
 * ----------
 * Alokasikan PID baru.
 */
static pid_t _alloc_pid(void)
{
    pid_t pid;

    pid = g_pid_counter++;

    /* Wrap around */
    if (g_pid_counter <= 0) {
        g_pid_counter = 1;
    }

    return pid;
}

/*
 * _alloc_tid
 * ----------
 * Alokasikan TID baru.
 */
static tid_t _alloc_tid(void)
{
    tid_t tid;

    tid = g_tid_counter++;

    if (g_tid_counter <= 0) {
        g_tid_counter = 1;
    }

    return tid;
}

/*
 * _find_free_proses
 * -----------------
 * Cari slot proses kosong.
 */
static proses_t *_find_free_proses(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < PROSES_MAX; i++) {
        if (g_proses_table[i].pid == 0) {
            return &g_proses_table[i];
        }
    }

    return NULL;
}

/*
 * _find_free_thread
 * -----------------
 * Cari slot thread kosong.
 */
static thread_t *_find_free_thread(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < PROSES_MAX * THREAD_MAX; i++) {
        if (g_thread_table[i].tid == 0) {
            return &g_thread_table[i];
        }
    }

    return NULL;
}

/*
 * _get_proses_by_pid
 * ------------------
 * Dapatkan proses berdasarkan PID.
 */
static proses_t *_get_proses_by_pid(pid_t pid)
{
    tak_bertanda32_t i;

    for (i = 0; i < PROSES_MAX; i++) {
        if (g_proses_table[i].pid == pid) {
            return &g_proses_table[i];
        }
    }

    return NULL;
}

/*
 * _get_thread_by_tid
 * ------------------
 * Dapatkan thread berdasarkan TID.
 */
static thread_t *_get_thread_by_tid(tid_t tid)
{
    tak_bertanda32_t i;

    for (i = 0; i < PROSES_MAX * THREAD_MAX; i++) {
        if (g_thread_table[i].tid == tid) {
            return &g_thread_table[i];
        }
    }

    return NULL;
}

/*
 * _add_to_ready_queue
 * -------------------
 * Tambahkan thread ke ready queue.
 */
static void _add_to_ready_queue(thread_t *thread)
{
    prioritas_t prio;
    thread_t *head;

    if (thread == NULL) {
        return;
    }

    prio = thread->prioritas;
    head = g_ready_queue[prio];

    if (head == NULL) {
        g_ready_queue[prio] = thread;
        thread->next = thread;
        thread->prev = thread;
    } else {
        thread->prev = head->prev;
        thread->next = head;
        head->prev->next = thread;
        head->prev = thread;
    }

    thread->status = PROSES_STATUS_SIAP;
    thread->flags |= PROSES_FLAG_READY;
}

/*
 * _remove_from_ready_queue
 * ------------------------
 * Hapus thread dari ready queue.
 */
static void _remove_from_ready_queue(thread_t *thread)
{
    prioritas_t prio;

    if (thread == NULL) {
        return;
    }

    prio = thread->prioritas;

    if (thread->next == thread) {
        /* Hanya satu thread di queue */
        g_ready_queue[prio] = NULL;
    } else {
        thread->prev->next = thread->next;
        thread->next->prev = thread->prev;

        if (g_ready_queue[prio] == thread) {
            g_ready_queue[prio] = thread->next;
        }
    }

    thread->next = NULL;
    thread->prev = NULL;
    thread->flags &= ~PROSES_FLAG_READY;
}

/*
 * _get_next_thread
 * ----------------
 * Dapatkan thread berikutnya untuk dijalankan.
 */
static thread_t *_get_next_thread(void)
{
    tak_bertanda32_t i;
    thread_t *thread;

    /* Scan dari prioritas tertinggi */
    for (i = PRIORITAS_REALTIME; i <= PRIORITAS_RENDAH; i--) {
        if (g_ready_queue[i] != NULL) {
            thread = g_ready_queue[i];
            _remove_from_ready_queue(thread);
            return thread;
        }
    }

    /* Tidak ada thread ready, kembalikan idle */
    return &g_idle_thread;
}

/*
 * _idle_thread_func
 * -----------------
 * Fungsi idle thread.
 */
static void _idle_thread_func(void)
{
    while (1) {
        __asm__ __volatile__("wfi");
        proses_yield();
    }
}

/*
 * _setup_thread_context
 * ---------------------
 * Setup context thread baru.
 */
static void _setup_thread_context(thread_t *thread,
                                   void (*entry)(void),
                                   void *stack_top,
                                   bool_t is_kernel)
{
    /* Clear context */
    kernel_memset(&thread->context, 0, sizeof(cpu_context_t));

    /* Set PC */
    thread->context.pc = (tak_bertanda64_t)entry;

    /* Set SP */
    thread->context.sp = (tak_bertanda64_t)stack_top;

    /* Set SPSR */
    if (is_kernel) {
        /* Kernel thread: EL1h */
        thread->context.spsr = SPSR_EL1h | SPSR_D | SPSR_A | SPSR_I | SPSR_F;
    } else {
        /* User thread: EL0t */
        thread->context.spsr = SPSR_EL0t;
    }

    /* Set flags */
    if (is_kernel) {
        thread->flags |= PROSES_FLAG_KERNEL;
    }
}

/*
 * ============================================================================
 * FUNGSI CONTEXT SWITCH (CONTEXT SWITCH FUNCTIONS)
 * ============================================================================
 */

/*
 * proses_context_save
 * -------------------
 * Simpan context CPU.
 * Dipanggil dari assembly.
 */
void proses_context_save(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    __asm__ __volatile__(
        "stp x19, x20, [%0, #0]\n\t"
        "stp x21, x22, [%0, #16]\n\t"
        "stp x23, x24, [%0, #32]\n\t"
        "stp x25, x26, [%0, #48]\n\t"
        "stp x27, x28, [%0, #64]\n\t"
        "stp x29, x30, [%0, #80]\n\t"
        "mov x1, sp\n\t"
        "str x1, [%0, #96]\n\t"
        :
        : "r"(ctx)
        : "x1", "memory"
    );
}

/*
 * proses_context_restore
 * ----------------------
 * Restore context CPU.
 * Dipanggil dari assembly.
 */
void proses_context_restore(cpu_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    __asm__ __volatile__(
        "ldp x19, x20, [%0, #0]\n\t"
        "ldp x21, x22, [%0, #16]\n\t"
        "ldp x23, x24, [%0, #32]\n\t"
        "ldp x25, x26, [%0, #48]\n\t"
        "ldp x27, x28, [%0, #64]\n\t"
        "ldp x29, x30, [%0, #80]\n\t"
        "ldr x1, [%0, #96]\n\t"
        "mov sp, x1\n\t"
        :
        : "r"(ctx)
        : "x1", "x19", "x20", "x21", "x22", "x23", "x24",
          "x25", "x26", "x27", "x28", "x29", "x30", "memory"
    );
}

/*
 * proses_switch_to
 * ----------------
 * Switch ke thread tertentu.
 */
void proses_switch_to(thread_t *next)
{
    thread_t *prev;

    if (next == NULL || next == g_thread_current) {
        return;
    }

    prev = g_thread_current;
    g_thread_current = next;

    /* Update status */
    prev->status = PROSES_STATUS_SIAP;
    prev->flags &= ~PROSES_FLAG_RUNNING;

    next->status = PROSES_STATUS_JALAN;
    next->flags |= PROSES_FLAG_RUNNING;

    /* Context switch via assembly */
    __asm__ __volatile__(
        /* Save current context */
        "stp x19, x20, [%0, #0]\n\t"
        "stp x21, x22, [%0, #16]\n\t"
        "stp x23, x24, [%0, #32]\n\t"
        "stp x25, x26, [%0, #48]\n\t"
        "stp x27, x28, [%0, #64]\n\t"
        "stp x29, x30, [%0, #80]\n\t"
        "mov x1, sp\n\t"
        "str x1, [%0, #96]\n\t"
        "mov x1, lr\n\t"
        "str x1, [%0, #104]\n\t"
        /* Restore new context */
        "ldp x19, x20, [%1, #0]\n\t"
        "ldp x21, x22, [%1, #16]\n\t"
        "ldp x23, x24, [%1, #32]\n\t"
        "ldp x25, x26, [%1, #48]\n\t"
        "ldp x27, x28, [%1, #64]\n\t"
        "ldp x29, x30, [%1, #80]\n\t"
        "ldr x1, [%1, #96]\n\t"
        "mov sp, x1\n\t"
        "ldr x1, [%1, #104]\n\t"
        "mov lr, x1\n\t"
        :
        : "r"(&prev->context), "r"(&next->context)
        : "x1", "x19", "x20", "x21", "x22", "x23", "x24",
          "x25", "x26", "x27", "x28", "x29", "x30", "memory"
    );
}

/*
 * ============================================================================
 * FUNGSI PUBLIC (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * proses_init
 * -----------
 * Inisialisasi subsistem proses.
 */
status_t proses_init(void)
{
    tak_bertanda32_t i;

    if (g_proses_diinisalisasi) {
        return STATUS_BERHASIL;
    }

    /* Clear process table */
    kernel_memset(g_proses_table, 0, sizeof(g_proses_table));

    /* Clear thread table */
    kernel_memset(g_thread_table, 0, sizeof(g_thread_table));

    /* Clear ready queues */
    for (i = 0; i <= PRIORITAS_REALTIME; i++) {
        g_ready_queue[i] = NULL;
    }

    /* Setup idle thread */
    kernel_memset(&g_idle_thread, 0, sizeof(thread_t));
    g_idle_thread.tid = _alloc_tid();
    g_idle_thread.status = PROSES_STATUS_JALAN;
    g_idle_thread.prioritas = PRIORITAS_RENDAH;
    g_idle_thread.flags = PROSES_FLAG_KERNEL | PROSES_FLAG_RUNNING;

    /* Allocate idle thread stack */
    g_idle_thread.stack = memori_alloc_halaman();
    g_idle_thread.stack_size = UKURAN_HALAMAN;

    /* Setup idle thread context */
    _setup_thread_context(&g_idle_thread, _idle_thread_func,
                          (void *)((tak_bertanda64_t)g_idle_thread.stack +
                                   g_idle_thread.stack_size),
                          BENAR);

    g_thread_current = &g_idle_thread;

    g_proses_diinisalisasi = BENAR;

    kernel_printf("[PROS-ARM64] Subsistem proses diinisialisasi\n");

    return STATUS_BERHASIL;
}

/*
 * proses_create
 * -------------
 * Buat proses baru.
 */
pid_t proses_create(const char *nama, prioritas_t prioritas)
{
    proses_t *proses;
    tak_bertanda32_t i;

    if (!g_proses_diinisalisasi) {
        return PID_INVALID;
    }

    proses = _find_free_proses();
    if (proses == NULL) {
        return PID_INVALID;
    }

    /* Initialize process */
    proses->pid = _alloc_pid();
    proses->ppid = (g_proses_current != NULL) ? g_proses_current->pid : 0;
    proses->prioritas = prioritas;
    proses->status = PROSES_STATUS_SIAP;
    proses->thread_count = 0;
    proses->mem_used = 0;

    /* Clear threads */
    for (i = 0; i < THREAD_MAX; i++) {
        proses->threads[i] = NULL;
    }

    /* Copy name */
    if (nama != NULL) {
        kernel_strncpy(proses->nama, nama, sizeof(proses->nama) - 1);
        proses->nama[sizeof(proses->nama) - 1] = '\0';
    } else {
        kernel_snprintf(proses->nama, sizeof(proses->nama),
                        "proses_%u", proses->pid);
    }

    /* Allocate page table */
    proses->pgd = (tak_bertanda64_t)memori_alloc_halaman();
    if (proses->pgd == 0) {
        proses->pid = 0;
        return PID_INVALID;
    }

    proses->asid = memori_get_asid();

    kernel_printf("[PROS-ARM64] Proses baru: %s (PID=%u)\n",
        proses->nama, proses->pid);

    return proses->pid;
}

/*
 * proses_destroy
 * --------------
 * Hancurkan proses.
 */
status_t proses_destroy(pid_t pid)
{
    proses_t *proses;
    tak_bertanda32_t i;

    if (!g_proses_diinisalisasi) {
        return STATUS_GAGAL;
    }

    proses = _get_proses_by_pid(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Destroy all threads */
    for (i = 0; i < THREAD_MAX; i++) {
        if (proses->threads[i] != NULL) {
            thread_destroy(proses->threads[i]->tid);
            proses->threads[i] = NULL;
        }
    }

    /* Free page table */
    if (proses->pgd != 0) {
        memori_free_halaman((void *)proses->pgd);
    }

    kernel_printf("[PROS-ARM64] Proses dihancurkan: PID=%u\n", pid);

    /* Clear process entry */
    kernel_memset(proses, 0, sizeof(proses_t));

    return STATUS_BERHASIL;
}

/*
 * proses_get_current
 * ------------------
 * Dapatkan proses saat ini.
 */
proses_t *proses_get_current(void)
{
    return g_proses_current;
}

/*
 * proses_get_pid
 * --------------
 * Dapatkan PID proses saat ini.
 */
pid_t proses_get_pid(void)
{
    if (g_proses_current == NULL) {
        return 0;
    }

    return g_proses_current->pid;
}

/*
 * thread_create
 * -------------
 * Buat thread baru.
 */
tid_t thread_create(pid_t pid, void (*entry)(void *), void *arg,
                     prioritas_t prioritas)
{
    proses_t *proses;
    thread_t *thread;
    void *stack;

    if (!g_proses_diinisalisasi) {
        return TID_INVALID;
    }

    proses = _get_proses_by_pid(pid);
    if (proses == NULL) {
        return TID_INVALID;
    }

    if (proses->thread_count >= THREAD_MAX) {
        return TID_INVALID;
    }

    thread = _find_free_thread();
    if (thread == NULL) {
        return TID_INVALID;
    }

    /* Initialize thread */
    thread->tid = _alloc_tid();
    thread->pid = pid;
    thread->prioritas = prioritas;
    thread->status = PROSES_STATUS_SIAP;
    thread->waktu_cpu = 0;

    /* Allocate stack */
    stack = memori_alloc_halaman();
    if (stack == NULL) {
        thread->tid = 0;
        return TID_INVALID;
    }

    thread->stack = stack;
    thread->stack_size = UKURAN_HALAMAN;

    /* Setup context */
    _setup_thread_context(thread, (void (*)(void))entry,
                          (void *)((tak_bertanda64_t)stack +
                                   thread->stack_size),
                          (proses->flags & PROSES_FLAG_KERNEL));

    /* Set argument in x0 */
    thread->context.x19 = (tak_bertanda64_t)arg;

    /* Add to process */
    proses->threads[proses->thread_count++] = thread;

    /* Add to ready queue */
    _add_to_ready_queue(thread);

    kernel_printf("[PROS-ARM64] Thread baru: TID=%u untuk PID=%u\n",
        thread->tid, pid);

    return thread->tid;
}

/*
 * thread_destroy
 * --------------
 * Hancurkan thread.
 */
status_t thread_destroy(tid_t tid)
{
    thread_t *thread;
    proses_t *proses;
    tak_bertanda32_t i;

    if (!g_proses_diinisalisasi) {
        return STATUS_GAGAL;
    }

    thread = _get_thread_by_tid(tid);
    if (thread == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Remove from ready queue */
    _remove_from_ready_queue(thread);

    /* Free stack */
    if (thread->stack != NULL) {
        memori_free_halaman(thread->stack);
    }

    /* Remove from process */
    proses = _get_proses_by_pid(thread->pid);
    if (proses != NULL) {
        for (i = 0; i < THREAD_MAX; i++) {
            if (proses->threads[i] == thread) {
                proses->threads[i] = NULL;
                proses->thread_count--;
                break;
            }
        }
    }

    kernel_printf("[PROS-ARM64] Thread dihancurkan: TID=%u\n", tid);

    /* Clear thread entry */
    kernel_memset(thread, 0, sizeof(thread_t));

    return STATUS_BERHASIL;
}

/*
 * thread_get_current
 * ------------------
 * Dapatkan thread saat ini.
 */
thread_t *thread_get_current(void)
{
    return g_thread_current;
}

/*
 * thread_get_tid
 * --------------
 * Dapatkan TID thread saat ini.
 */
tid_t thread_get_tid(void)
{
    if (g_thread_current == NULL) {
        return 0;
    }

    return g_thread_current->tid;
}

/*
 * proses_yield
 * ------------
 * Yield CPU ke thread lain.
 */
void proses_yield(void)
{
    thread_t *next;

    if (!g_proses_diinisalisasi || g_thread_current == NULL) {
        return;
    }

    /* Add current thread back to ready queue if not idle */
    if (g_thread_current != &g_idle_thread) {
        _add_to_ready_queue(g_thread_current);
    }

    /* Get next thread */
    next = _get_next_thread();

    /* Switch */
    if (next != NULL && next != g_thread_current) {
        proses_switch_to(next);
    }
}

/*
 * proses_exit
 * -----------
 * Exit proses saat ini.
 */
void proses_exit(tak_bertanda32_t exit_code)
{
    pid_t pid;

    (void)exit_code;

    if (g_proses_current == NULL) {
        return;
    }

    pid = g_proses_current->pid;

    kernel_printf("[PROS-ARM64] Proses exit: PID=%u\n", pid);

    proses_destroy(pid);

    /* Schedule next process */
    proses_yield();
}

/*
 * thread_exit
 * -----------
 * Exit thread saat ini.
 */
void thread_exit(tak_bertanda32_t exit_code)
{
    tid_t tid;

    (void)exit_code;

    if (g_thread_current == NULL) {
        return;
    }

    tid = g_thread_current->tid;

    kernel_printf("[PROS-ARM64] Thread exit: TID=%u\n", tid);

    thread_destroy(tid);

    /* Schedule next thread */
    proses_yield();
}

/*
 * proses_sleep
 * ------------
 * Sleep proses untuk waktu tertentu.
 */
void proses_sleep(tak_bertanda64_t milidetik)
{
    tak_bertanda64_t target;

    target = hal_timer_get_ticks() +
             (milidetik * FREKUENSI_TIMER / 1000);

    while (hal_timer_get_ticks() < target) {
        proses_yield();
    }
}

/*
 * proses_block
 * ------------
 * Block proses saat ini.
 */
void proses_block(void)
{
    if (g_thread_current != NULL) {
        g_thread_current->status = PROSES_STATUS_TUNGGU;
        _remove_from_ready_queue(g_thread_current);
        proses_yield();
    }
}

/*
 * proses_unblock
 * --------------
 * Unblock thread.
 */
void proses_unblock(tid_t tid)
{
    thread_t *thread;

    thread = _get_thread_by_tid(tid);
    if (thread != NULL && thread->status == PROSES_STATUS_TUNGGU) {
        _add_to_ready_queue(thread);
    }
}

/*
 * proses_set_priority
 * -------------------
 * Set prioritas thread.
 */
status_t proses_set_priority(tid_t tid, prioritas_t prioritas)
{
    thread_t *thread;

    thread = _get_thread_by_tid(tid);
    if (thread == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Remove from old queue */
    if (thread->flags & PROSES_FLAG_READY) {
        _remove_from_ready_queue(thread);
    }

    /* Update priority */
    thread->prioritas = prioritas;

    /* Add to new queue */
    if (thread->flags & PROSES_FLAG_READY) {
        _add_to_ready_queue(thread);
    }

    return STATUS_BERHASIL;
}

/*
 * proses_get_priority
 * -------------------
 * Dapatkan prioritas thread.
 */
prioritas_t proses_get_priority(tid_t tid)
{
    thread_t *thread;

    thread = _get_thread_by_tid(tid);
    if (thread == NULL) {
        return PRIORITAS_NORMAL;
    }

    return thread->prioritas;
}

/*
 * proses_get_waktu_cpu
 * --------------------
 * Dapatkan waktu CPU thread.
 */
tak_bertanda64_t proses_get_waktu_cpu(tid_t tid)
{
    thread_t *thread;

    thread = _get_thread_by_tid(tid);
    if (thread == NULL) {
        return 0;
    }

    return thread->waktu_cpu;
}

/*
 * proses_scheduler_tick
 * ---------------------
 * Scheduler tick.
 */
void proses_scheduler_tick(void)
{
    if (g_thread_current != NULL) {
        g_thread_current->waktu_cpu++;

        /* Preempt if timeslice expired */
        /* For simplicity, yield every tick for round-robin */
        proses_yield();
    }
}

/*
 * proses_get_count
 * ----------------
 * Dapatkan jumlah proses aktif.
 */
tak_bertanda32_t proses_get_count(void)
{
    tak_bertanda32_t count = 0;
    tak_bertanda32_t i;

    for (i = 0; i < PROSES_MAX; i++) {
        if (g_proses_table[i].pid != 0) {
            count++;
        }
    }

    return count;
}

/*
 * proses_get_thread_count
 * -----------------------
 * Dapatkan jumlah thread aktif.
 */
tak_bertanda32_t proses_get_thread_count(void)
{
    tak_bertanda32_t count = 0;
    tak_bertanda32_t i;

    for (i = 0; i < PROSES_MAX * THREAD_MAX; i++) {
        if (g_thread_table[i].tid != 0) {
            count++;
        }
    }

    return count;
}
