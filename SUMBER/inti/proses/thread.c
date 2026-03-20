/*
 * PIGURA OS - THREAD.C
 * ---------------------
 * Implementasi manajemen thread dalam proses.
 *
 * Berkas ini berisi fungsi-fungsi untuk membuat, mengelola, dan
 * menghancurkan thread dalam konteks proses Pigura OS.
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

/* Jumlah maksimum thread per proses */
#define THREAD_MAKS_PER_PROSES    CONFIG_MAKS_THREAD

/* Ukuran stack thread kernel */
#define THREAD_STACK_KERNEL       CONFIG_STACK_KERNEL

/* Ukuran stack thread user */
#define THREAD_STACK_USER         CONFIG_STACK_USER

/* Flag thread */
#define THREAD_FLAG_NONE          0x00
#define THREAD_FLAG_KERNEL        0x01
#define THREAD_FLAG_DETACHED      0x02
#define THREAD_FLAG_JOINED        0x04

/* Magic number thread */
#define THREAD_MAGIC              0x54485244  /* "THRD" */

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur thread internal */
typedef struct thread_internal {
    tak_bertanda32_t magic;           /* Magic number untuk validasi */
    tak_bertanda32_t tid;             /* Thread ID */
    tak_bertanda32_t pid;             /* Process ID parent */
    proses_status_t status;           /* Status thread */
    tak_bertanda32_t flags;           /* Flag thread */
    prioritas_t prioritas;            /* Prioritas thread */
    tanda32_t exit_code;              /* Exit code */

    /* Context */
    void *context;                    /* Pointer ke CPU context */
    void *stack;                      /* Stack pointer */
    ukuran_t stack_size;              /* Ukuran stack */

    /* Scheduling */
    tak_bertanda32_t quantum;         /* Quantum remaining */
    tak_bertanda64_t cpu_time;        /* CPU time used */

    /* Linked list */
    struct thread_internal *next;     /* Thread berikutnya */
    struct thread_internal *prev;     /* Thread sebelumnya */

    /* Join/Wait */
    struct thread_internal *waiting;  /* Thread yang menunggu join */
    void *retval;                     /* Return value */

    /* Thread-local storage */
    void *tls;                        /* Thread-local storage */
} thread_internal_t;

/* Pool thread */
typedef struct {
    thread_internal_t pool[CONFIG_MAKS_PROSES * THREAD_MAKS_PER_PROSES];
    thread_internal_t *free_list;
    tak_bertanda32_t count;
    spinlock_t lock;
} thread_pool_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Pool thread global */
static thread_pool_t thread_pool = {0};

/* Status inisialisasi */
static bool_t thread_initialized = SALAH;

/* TID berikutnya */
static tak_bertanda32_t next_tid = 1;

/* Statistik thread */
static struct {
    tak_bertanda64_t created;
    tak_bertanda64_t exited;
    tak_bertanda64_t active;
} thread_stats = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * thread_pool_init
 * ----------------
 * Inisialisasi pool thread.
 */
static void thread_pool_init(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t total;

    total = CONFIG_MAKS_PROSES * THREAD_MAKS_PER_PROSES;

    thread_pool.free_list = NULL;
    thread_pool.count = 0;
    spinlock_init(&thread_pool.lock);

    for (i = 0; i < total; i++) {
        thread_pool.pool[i].magic = THREAD_MAGIC;
        thread_pool.pool[i].tid = 0;
        thread_pool.pool[i].next = thread_pool.free_list;
        thread_pool.free_list = &thread_pool.pool[i];
    }

    thread_pool.count = total;
}

/*
 * thread_alloc
 * ------------
 * Alokasikan struktur thread dari pool.
 *
 * Return: Pointer ke thread, atau NULL
 */
static thread_internal_t *thread_alloc(void)
{
    thread_internal_t *thread;

    if (thread_pool.free_list == NULL) {
        return NULL;
    }

    thread = thread_pool.free_list;
    thread_pool.free_list = thread->next;
    thread_pool.count--;

    /* Reset struktur */
    thread->tid = next_tid++;
    thread->pid = 0;
    thread->status = PROSES_STATUS_BELUM;
    thread->flags = THREAD_FLAG_NONE;
    thread->prioritas = PRIORITAS_NORMAL;
    thread->exit_code = 0;
    thread->context = NULL;
    thread->stack = NULL;
    thread->stack_size = 0;
    thread->quantum = QUANTUM_DEFAULT;
    thread->cpu_time = 0;
    thread->next = NULL;
    thread->prev = NULL;
    thread->waiting = NULL;
    thread->retval = NULL;
    thread->tls = NULL;

    return thread;
}

/*
 * thread_free
 * -----------
 * Kembalikan struktur thread ke pool.
 *
 * Parameter:
 *   thread - Pointer ke thread
 */
static void thread_free(thread_internal_t *thread)
{
    if (thread == NULL) {
        return;
    }

    thread->magic = THREAD_MAGIC;
    thread->tid = 0;
    thread->next = thread_pool.free_list;
    thread_pool.free_list = thread;
    thread_pool.count++;
}

/*
 * thread_alloc_stack
 * ------------------
 * Alokasikan stack untuk thread.
 *
 * Parameter:
 *   size    - Ukuran stack
 *   is_user - Apakah stack untuk user mode
 *
 * Return: Pointer ke stack, atau NULL
 */
static void *thread_alloc_stack(ukuran_t size, bool_t is_user)
{
    void *stack;

    if (size == 0) {
        size = is_user ? THREAD_STACK_USER : THREAD_STACK_KERNEL;
    }

    /* Ratakan ke ukuran halaman */
    size = RATAKAN_ATAS(size, UKURAN_HALAMAN);

    /* Alokasikan halaman fisik */
    if (is_user) {
        /* Untuk user thread, alokasi melalui VM proses */
        proses_t *proses = proses_get_current();
        if (proses != NULL && proses->vm != NULL) {
            alamat_virtual_t addr;
            addr = vm_map(proses->vm, 0, size, 0x03, VMA_FLAG_STACK);
            if (addr == 0) {
                return NULL;
            }
            return (void *)(addr + size);  /* Stack grows down */
        }
        return NULL;
    } else {
        /* Untuk kernel thread */
        stack = kmalloc(size);
        if (stack == NULL) {
            return NULL;
        }
        return (void *)((tak_bertanda8_t *)stack + size);
    }
}

/*
 * thread_free_stack
 * -----------------
 * Bebaskan stack thread.
 *
 * Parameter:
 *   stack   - Pointer ke stack
 *   size    - Ukuran stack
 *   is_user - Apakah stack user mode
 */
static void thread_free_stack(void *stack, ukuran_t size, bool_t is_user)
{
    if (stack == NULL) {
        return;
    }

    if (size == 0) {
        size = is_user ? THREAD_STACK_USER : THREAD_STACK_KERNEL;
    }

    size = RATAKAN_ATAS(size, UKURAN_HALAMAN);

    if (is_user) {
        proses_t *proses = proses_get_current();
        if (proses != NULL && proses->vm != NULL) {
            alamat_virtual_t base = (alamat_virtual_t)stack - size;
            vm_unmap(proses->vm, base, size);
        }
    } else {
        /* Stack kernel dialokasi dengan kmalloc */
        kfree((void *)((tak_bertanda8_t *)stack - size));
    }
}

/*
 * thread_entry_wrapper
 * --------------------
 * Wrapper untuk entry point thread.
 * Memastikan thread exit dengan benar.
 *
 * Parameter:
 *   entry - Entry point thread
 *   arg   - Argumen thread
 */
static void thread_entry_wrapper(void (*entry)(void *), void *arg)
{
    /* Panggil entry point */
    if (entry != NULL) {
        entry(arg);
    }

    /* Thread exit */
    thread_exit(NULL);
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * thread_init
 * -----------
 * Inisialisasi subsistem thread.
 *
 * Return: Status operasi
 */
status_t thread_init(void)
{
    if (thread_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Init pool */
    thread_pool_init();

    /* Reset stats */
    kernel_memset(&thread_stats, 0, sizeof(thread_stats));

    thread_initialized = BENAR;

    kernel_printf("[THREAD] Thread manager initialized\n");
    kernel_printf("         Pool size: %lu threads\n",
                  (tak_bertanda64_t)(CONFIG_MAKS_PROSES *
                                     THREAD_MAKS_PER_PROSES));

    return STATUS_BERHASIL;
}

/*
 * thread_create
 * -------------
 * Buat thread baru dalam proses.
 *
 * Parameter:
 *   pid     - Process ID
 *   entry   - Entry point thread
 *   arg     - Argumen thread
 *   flags   - Flag thread
 *
 * Return: Thread ID, atau -1 jika gagal
 */
tak_bertanda32_t thread_create(tak_bertanda32_t pid,
                               void (*entry)(void *), void *arg,
                               tak_bertanda32_t flags)
{
    thread_internal_t *thread;
    proses_t *proses;
    bool_t is_kernel;
    void *stack;
    ukuran_t stack_size;
    void *context;

    if (!thread_initialized || entry == NULL) {
        return TID_INVALID;
    }

    /* Cari proses parent */
    proses = proses_cari(pid);
    if (proses == NULL) {
        return TID_INVALID;
    }

    /* Cek batas thread */
    if (proses->thread_count >= THREAD_MAKS_PER_PROSES) {
        return TID_INVALID;
    }

    spinlock_kunci(&thread_pool.lock);

    /* Alokasikan struktur thread */
    thread = thread_alloc();
    if (thread == NULL) {
        spinlock_buka(&thread_pool.lock);
        return TID_INVALID;
    }

    /* Tentukan tipe thread */
    is_kernel = (flags & THREAD_FLAG_KERNEL) ? BENAR : SALAH;

    /* Set properties */
    thread->pid = pid;
    thread->flags = flags;
    thread->prioritas = proses->prioritas;

    /* Alokasikan stack */
    stack_size = is_kernel ? THREAD_STACK_KERNEL : THREAD_STACK_USER;
    stack = thread_alloc_stack(stack_size, !is_kernel);

    if (stack == NULL) {
        thread_free(thread);
        spinlock_buka(&thread_pool.lock);
        return TID_INVALID;
    }

    thread->stack = stack;
    thread->stack_size = stack_size;

    /* Buat context */
    context = context_create((alamat_virtual_t)thread_entry_wrapper,
                             stack, !is_kernel);

    if (context == NULL) {
        thread_free_stack(thread->stack, thread->stack_size, !is_kernel);
        thread_free(thread);
        spinlock_buka(&thread_pool.lock);
        return TID_INVALID;
    }

    /* Setup context dengan argumen */
    if (is_kernel) {
        /* Push argumen untuk kernel thread */
        tak_bertanda32_t *sp = (tak_bertanda32_t *)stack;
        sp--;
        *sp = (tak_bertanda32_t)(alamat_ptr_t)arg;
        sp--;
        *sp = (tak_bertanda32_t)(alamat_ptr_t)entry;
        context_set_stack(context, sp);
    }

    thread->context = context;
    thread->status = PROSES_STATUS_SIAP;

    /* Tambahkan ke list proses */
    thread->next = (thread_internal_t *)proses->threads;
    if (proses->threads != NULL) {
        proses->threads->prev = thread;
    }
    proses->threads = (void *)thread;
    proses->thread_count++;

    /* Update stats */
    thread_stats.created++;
    thread_stats.active++;

    spinlock_buka(&thread_pool.lock);

    /* Tambahkan ke scheduler */
    scheduler_tambah_proses(proses);

    return thread->tid;
}

/*
 * thread_exit
 * -----------
 * Exit thread saat ini.
 *
 * Parameter:
 *   retval - Return value
 */
void thread_exit(void *retval)
{
    thread_internal_t *thread;
    proses_t *proses;

    if (!thread_initialized) {
        return;
    }

    proses = proses_get_current();
    if (proses == NULL || proses->main_thread == NULL) {
        return;
    }

    thread = (thread_internal_t *)proses->main_thread;

    spinlock_kunci(&thread_pool.lock);

    /* Set retval dan status */
    thread->retval = retval;
    thread->status = PROSES_STATUS_ZOMBIE;

    /* Jika ada thread yang menunggu join, wake up */
    if (thread->waiting != NULL) {
        scheduler_unblock((proses_t *)thread->waiting);
    }

    /* Update stats */
    thread_stats.exited++;
    if (thread_stats.active > 0) {
        thread_stats.active--;
    }

    spinlock_buka(&thread_pool.lock);

    /* Hapus dari scheduler */
    scheduler_hapus_proses(proses);

    /* Schedule ke thread lain */
    scheduler_schedule();

    /* Tidak seharusnya sampai sini */
    kernel_panic("thread_exit: should not reach here");
}

/*
 * thread_join
 * -----------
 * Tunggu thread selesai.
 *
 * Parameter:
 *   tid    - Thread ID
 *   retval - Pointer untuk return value
 *
 * Return: Status operasi
 */
status_t thread_join(tak_bertanda32_t tid, void **retval)
{
    thread_internal_t *thread;
    proses_t *proses;
    thread_internal_t *curr;

    if (!thread_initialized) {
        return STATUS_GAGAL;
    }

    proses = proses_get_current();
    if (proses == NULL) {
        return STATUS_GAGAL;
    }

    spinlock_kunci(&thread_pool.lock);

    /* Cari thread dengan TID */
    thread = NULL;
    curr = (thread_internal_t *)proses->threads;

    while (curr != NULL) {
        if (curr->tid == tid) {
            thread = curr;
            break;
        }
        curr = curr->next;
    }

    if (thread == NULL) {
        spinlock_buka(&thread_pool.lock);
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Cek jika sudah detached */
    if (thread->flags & THREAD_FLAG_DETACHED) {
        spinlock_buka(&thread_pool.lock);
        return STATUS_GAGAL;
    }

    /* Cek jika sudah selesai */
    if (thread->status == PROSES_STATUS_ZOMBIE) {
        if (retval != NULL) {
            *retval = thread->retval;
        }
        thread_free(thread);
        spinlock_buka(&thread_pool.lock);
        return STATUS_BERHASIL;
    }

    /* Tandai bahwa kita menunggu */
    thread->waiting = (thread_internal_t *)proses;
    thread->flags |= THREAD_FLAG_JOINED;

    spinlock_buka(&thread_pool.lock);

    /* Block sampai thread selesai */
    scheduler_block(0);

    /* Thread sudah selesai */
    spinlock_kunci(&thread_pool.lock);

    if (retval != NULL) {
        *retval = thread->retval;
    }

    thread_free(thread);

    spinlock_buka(&thread_pool.lock);

    return STATUS_BERHASIL;
}

/*
 * thread_detach
 * -------------
 * Detach thread.
 *
 * Parameter:
 *   tid - Thread ID
 *
 * Return: Status operasi
 */
status_t thread_detach(tak_bertanda32_t tid)
{
    thread_internal_t *thread;
    proses_t *proses;
    thread_internal_t *curr;

    if (!thread_initialized) {
        return STATUS_GAGAL;
    }

    proses = proses_get_current();
    if (proses == NULL) {
        return STATUS_GAGAL;
    }

    spinlock_kunci(&thread_pool.lock);

    /* Cari thread */
    thread = NULL;
    curr = (thread_internal_t *)proses->threads;

    while (curr != NULL) {
        if (curr->tid == tid) {
            thread = curr;
            break;
        }
        curr = curr->next;
    }

    if (thread == NULL) {
        spinlock_buka(&thread_pool.lock);
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Tidak bisa detach jika sudah di-join */
    if (thread->flags & THREAD_FLAG_JOINED) {
        spinlock_buka(&thread_pool.lock);
        return STATUS_GAGAL;
    }

    /* Set flag detached */
    thread->flags |= THREAD_FLAG_DETACHED;

    /* Jika thread sudah zombie, langsung cleanup */
    if (thread->status == PROSES_STATUS_ZOMBIE) {
        thread_free(thread);
    }

    spinlock_buka(&thread_pool.lock);

    return STATUS_BERHASIL;
}

/*
 * thread_yield
 * ------------
 * Yield CPU ke thread lain.
 */
void thread_yield(void)
{
    scheduler_yield();
}

/*
 * thread_self
 * -----------
 * Dapatkan TID thread saat ini.
 *
 * Return: Thread ID
 */
tak_bertanda32_t thread_self(void)
{
    proses_t *proses;

    proses = proses_get_current();
    if (proses == NULL || proses->main_thread == NULL) {
        return TID_INVALID;
    }

    return ((thread_internal_t *)proses->main_thread)->tid;
}

/*
 * thread_set_priority
 * -------------------
 * Set prioritas thread.
 *
 * Parameter:
 *   tid       - Thread ID
 *   prioritas - Prioritas baru
 *
 * Return: Status operasi
 */
status_t thread_set_priority(tak_bertanda32_t tid, prioritas_t prioritas)
{
    thread_internal_t *thread;
    proses_t *proses;
    thread_internal_t *curr;

    if (!thread_initialized) {
        return STATUS_GAGAL;
    }

    if (prioritas > PRIORITAS_REALTIME) {
        return STATUS_PARAM_INVALID;
    }

    proses = proses_get_current();
    if (proses == NULL) {
        return STATUS_GAGAL;
    }

    spinlock_kunci(&thread_pool.lock);

    /* Cari thread */
    thread = NULL;
    curr = (thread_internal_t *)proses->threads;

    while (curr != NULL) {
        if (curr->tid == tid) {
            thread = curr;
            break;
        }
        curr = curr->next;
    }

    if (thread == NULL) {
        spinlock_buka(&thread_pool.lock);
        return STATUS_TIDAK_DITEMUKAN;
    }

    thread->prioritas = prioritas;
    thread->quantum = dapatkan_quantum(prioritas);

    spinlock_buka(&thread_pool.lock);

    return STATUS_BERHASIL;
}

/*
 * thread_get_priority
 * -------------------
 * Dapatkan prioritas thread.
 *
 * Parameter:
 *   tid - Thread ID
 *
 * Return: Prioritas thread, atau -1 jika error
 */
tanda32_t thread_get_priority(tak_bertanda32_t tid)
{
    thread_internal_t *thread;
    proses_t *proses;
    thread_internal_t *curr;
    tanda32_t result;

    if (!thread_initialized) {
        return -1;
    }

    proses = proses_get_current();
    if (proses == NULL) {
        return -1;
    }

    spinlock_kunci(&thread_pool.lock);

    /* Cari thread */
    thread = NULL;
    curr = (thread_internal_t *)proses->threads;

    while (curr != NULL) {
        if (curr->tid == tid) {
            thread = curr;
            break;
        }
        curr = curr->next;
    }

    if (thread == NULL) {
        spinlock_buka(&thread_pool.lock);
        return -1;
    }

    result = (tanda32_t)thread->prioritas;

    spinlock_buka(&thread_pool.lock);

    return result;
}

/*
 * thread_get_stats
 * ----------------
 * Dapatkan statistik thread.
 *
 * Parameter:
 *   created - Pointer untuk jumlah thread dibuat
 *   exited  - Pointer untuk jumlah thread exit
 *   active  - Pointer untuk jumlah thread aktif
 */
void thread_get_stats(tak_bertanda64_t *created, tak_bertanda64_t *exited,
                      tak_bertanda64_t *active)
{
    if (created != NULL) {
        *created = thread_stats.created;
    }

    if (exited != NULL) {
        *exited = thread_stats.exited;
    }

    if (active != NULL) {
        *active = thread_stats.active;
    }
}

/*
 * thread_print_info
 * -----------------
 * Print informasi thread.
 *
 * Parameter:
 *   tid - Thread ID
 */
void thread_print_info(tak_bertanda32_t tid)
{
    thread_internal_t *thread;
    thread_internal_t *curr;
    proses_t *proses;
    const char *status_str;

    if (!thread_initialized) {
        kernel_printf("[THREAD] Thread manager tidak diinisialisasi\n");
        return;
    }

    /* Cari thread di semua proses */
    thread = NULL;

    for (proses = proses_get_kernel(); proses != NULL;
         proses = (proses_t *)proses->next) {

        curr = (thread_internal_t *)proses->threads;
        while (curr != NULL) {
            if (curr->tid == tid) {
                thread = curr;
                break;
            }
            curr = curr->next;
        }

        if (thread != NULL) {
            break;
        }
    }

    if (thread == NULL) {
        kernel_printf("[THREAD] Thread %lu tidak ditemukan\n",
                      (tak_bertanda64_t)tid);
        return;
    }

    /* Status string */
    switch (thread->status) {
        case PROSES_STATUS_BELUM:
            status_str = "Belum";
            break;
        case PROSES_STATUS_SIAP:
            status_str = "Siap";
            break;
        case PROSES_STATUS_JALAN:
            status_str = "Jalan";
            break;
        case PROSES_STATUS_TUNGGU:
            status_str = "Tunggu";
            break;
        case PROSES_STATUS_ZOMBIE:
            status_str = "Zombie";
            break;
        default:
            status_str = "Unknown";
            break;
    }

    kernel_printf("\n[THREAD] Informasi Thread:\n");
    kernel_printf("========================================\n");
    kernel_printf("  TID:        %lu\n", (tak_bertanda64_t)thread->tid);
    kernel_printf("  PID:        %lu\n", (tak_bertanda64_t)thread->pid);
    kernel_printf("  Status:     %s\n", status_str);
    kernel_printf("  Prioritas:  %d\n", (tanda32_t)thread->prioritas);
    kernel_printf("  Stack:      0x%08lX\n",
                  (tak_bertanda32_t)(alamat_ptr_t)thread->stack);
    kernel_printf("  Stack Size: %lu bytes\n",
                  (tak_bertanda64_t)thread->stack_size);
    kernel_printf("  CPU Time:   %lu ticks\n", thread->cpu_time);
    kernel_printf("  Flags:      0x%02X\n", thread->flags);
    kernel_printf("========================================\n");
}
