/*
 * PIGURA OS - FORK.C
 * -------------------
 * Implementasi duplikasi proses (fork).
 *
 * Berkas ini berisi fungsi-fungsi untuk menduplikasi proses,
 * membuat salinan address space, dan setup context untuk child.
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

/* Flag duplikasi */
#define FORK_FLAG_NONE          0x00
#define FORK_FLAG_SHARE_VM      0x01   /* Share address space */
#define FORK_FLAG_SHARE_FILES   0x02   /* Share file descriptors */
#define FORK_FLAG_SHARE_SIGNAL  0x04   /* Share signal handlers */
#define FORK_FLAG_VFORK         0x08   /* vfork semantics */

/* Return value untuk child */
#define FORK_CHILD_RETURN       0

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik fork */
static struct {
    tak_bertanda64_t total_forks;
    tak_bertanda64_t successful;
    tak_bertanda64_t failed;
    tak_bertanda64_t vforks;
} fork_stats = {0};

/* Status inisialisasi */
static bool_t fork_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * duplikasi_vm
 * ------------
 * Duplikasi address space proses.
 *
 * Parameter:
 *   parent - Pointer ke proses parent
 *   child  - Pointer ke proses child
 *
 * Return: Status operasi
 */
static status_t duplikasi_vm(proses_t *parent, proses_t *child)
{
    vm_descriptor_t *new_vm;
    alamat_virtual_t addr;
    alamat_fisik_t phys;
    tak_bertanda32_t flags;
    ukuran_t size;

    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Buat VM descriptor baru */
    new_vm = vm_create_address_space();
    if (new_vm == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    child->vm = new_vm;

    /* Copy semua mapping dari parent */
    if (parent->vm != NULL) {
        /* Iterasi semua VMA dan copy */
        addr = ALAMAT_USER_MULAI;

        while (addr < ALAMAT_USER_AKHIR) {
            /* Dapatkan mapping di alamat ini */
            if (vm_query(parent->vm, addr, &phys, &flags, &size)) {
                /* Map di child dengan flag yang sama */
                if (vm_map(new_vm, addr, size, flags, 0) == 0) {
                    vm_destroy_address_space(new_vm);
                    child->vm = NULL;
                    return STATUS_MEMORI_HABIS;
                }

                /* Copy konten halaman */
                kernel_memcpy((void *)addr, (void *)addr, size);
            }

            addr += size;
        }
    }

    return STATUS_BERHASIL;
}

/*
 * duplikasi_fd
 * ------------
 * Duplikasi file descriptors.
 *
 * Parameter:
 *   parent - Pointer ke proses parent
 *   child  - Pointer ke proses child
 *
 * Return: Status operasi
 */
static status_t duplikasi_fd(proses_t *parent, proses_t *child)
{
    tak_bertanda32_t i;

    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Copy file descriptor table */
    for (i = 0; i < MAKS_FD_PER_PROSES; i++) {
        if (parent->fd_table[i] != NULL) {
            /* Duplikasi referensi */
            child->fd_table[i] = parent->fd_table[i];

            /* Increment reference count */
            if (child->fd_table[i]->ops != NULL &&
                child->fd_table[i]->ops->dup != NULL) {
                child->fd_table[i]->ops->dup(child->fd_table[i]);
            }
        }
    }

    child->fd_count = parent->fd_count;

    return STATUS_BERHASIL;
}

/*
 * duplikasi_signal
 * ----------------
 * Duplikasi signal handlers.
 *
 * Parameter:
 *   parent - Pointer ke proses parent
 *   child  - Pointer ke proses child
 *
 * Return: Status operasi
 */
static status_t duplikasi_signal(proses_t *parent, proses_t *child)
{
    tak_bertanda32_t i;

    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Copy signal handlers */
    for (i = 0; i < MAKS_SIGNAL; i++) {
        child->signal_handlers[i] = parent->signal_handlers[i];
        child->signal_mask[i] = parent->signal_mask[i];
    }

    /* Clear pending signals */
    kernel_memset(child->signal_pending, 0, sizeof(child->signal_pending));

    return STATUS_BERHASIL;
}

/*
 * duplikasi_thread
 * ----------------
 * Duplikasi main thread.
 *
 * Parameter:
 *   parent - Pointer ke proses parent
 *   child  - Pointer ke proses child
 *
 * Return: Status operasi
 */
static status_t duplikasi_thread(proses_t *parent, proses_t *child)
{
    thread_t *parent_thread;
    thread_t *child_thread;
    void *stack;
    ukuran_t stack_size;

    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_INVALID;
    }

    parent_thread = parent->main_thread;
    if (parent_thread == NULL) {
        return STATUS_GAGAL;
    }

    /* Alokasikan thread baru */
    child_thread = thread_alloc();
    if (child_thread == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    /* Copy properties */
    child_thread->tid = child->pid;  /* TID = PID untuk main thread */
    child_thread->pid = child->pid;
    child_thread->status = PROSES_STATUS_SIAP;
    child_thread->prioritas = parent_thread->prioritas;
    child_thread->quantum = parent_thread->quantum;

    /* Alokasikan stack baru */
    stack_size = parent_thread->stack_size;
    stack = thread_alloc_stack(stack_size, BENAR);  /* User mode */

    if (stack == NULL) {
        thread_free(child_thread);
        return STATUS_MEMORI_HABIS;
    }

    child_thread->stack = stack;
    child_thread->stack_size = stack_size;

    /* Duplikasi context */
    child_thread->context = context_dup(parent_thread->context);
    if (child_thread->context == NULL) {
        thread_free_stack(stack, stack_size, BENAR);
        thread_free(child_thread);
        return STATUS_MEMORI_HABIS;
    }

    /* Set return value untuk child */
    context_set_return(child_thread->context, FORK_CHILD_RETURN);

    /* Set stack di context */
    context_set_stack(child_thread->context, stack);

    /* Set CR3 untuk child */
    if (child->vm != NULL) {
        context_set_cr3(child_thread->context, child->vm->cr3);
    }

    child->main_thread = child_thread;
    child->threads = child_thread;
    child->thread_count = 1;

    return STATUS_BERHASIL;
}

/*
 * setup_child_context
 * -------------------
 * Setup context untuk child process.
 *
 * Parameter:
 *   child - Pointer ke proses child
 *
 * Return: Status operasi
 */
static status_t setup_child_context(proses_t *child)
{
    thread_t *thread;

    if (child == NULL || child->main_thread == NULL) {
        return STATUS_PARAM_INVALID;
    }

    thread = child->main_thread;

    /* Set status siap */
    thread->status = PROSES_STATUS_SIAP;
    child->status = PROSES_STATUS_SIAP;

    /* Set return value 0 untuk child */
    if (thread->context != NULL) {
        context_set_return(thread->context, 0);
    }

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * fork_init
 * ---------
 * Inisialisasi subsistem fork.
 *
 * Return: Status operasi
 */
status_t fork_init(void)
{
    if (fork_initialized) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&fork_stats, 0, sizeof(fork_stats));
    fork_initialized = BENAR;

    kernel_printf("[FORK] Fork subsystem initialized\n");

    return STATUS_BERHASIL;
}

/*
 * do_fork
 * -------
 * Lakukan fork proses.
 *
 * Parameter:
 *   flags - Flag duplikasi
 *   stack - Stack baru (untuk vfork/clone, NULL untuk fork normal)
 *
 * Return: PID child di parent, 0 di child, -1 jika error
 */
pid_t do_fork(tak_bertanda32_t flags, void *stack)
{
    proses_t *parent;
    proses_t *child;
    pid_t child_pid;
    status_t status;

    if (!fork_initialized) {
        return PID_INVALID;
    }

    parent = proses_get_current();
    if (parent == NULL) {
        return PID_INVALID;
    }

    /* Tidak boleh fork kernel process */
    if (parent->flags & PROSES_FLAG_KERNEL) {
        return PID_INVALID;
    }

    fork_stats.total_forks++;

    /* Alokasikan struktur proses child */
    child_pid = proses_create(parent->nama, parent->pid, 0);
    if (child_pid == PID_INVALID) {
        fork_stats.failed++;
        return PID_INVALID;
    }

    child = proses_cari(child_pid);
    if (child == NULL) {
        fork_stats.failed++;
        return PID_INVALID;
    }

    /* Copy properties dari parent */
    child->gid = parent->gid;
    child->uid = parent->uid;
    child->prioritas = parent->prioritas;
    kernel_strncpy(child->cwd, parent->cwd, MAKS_PATH_LEN);
    kernel_strncpy(child->root, parent->root, MAKS_PATH_LEN);

    /* Duplikasi address space */
    if (!(flags & FORK_FLAG_SHARE_VM)) {
        status = duplikasi_vm(parent, child);
        if (status != STATUS_BERHASIL) {
            proses_hapus(child_pid);
            fork_stats.failed++;
            return PID_INVALID;
        }
    } else {
        child->vm = parent->vm;
    }

    /* Duplikasi file descriptors */
    if (!(flags & FORK_FLAG_SHARE_FILES)) {
        status = duplikasi_fd(parent, child);
        if (status != STATUS_BERHASIL) {
            if (!(flags & FORK_FLAG_SHARE_VM) && child->vm != NULL) {
                vm_destroy_address_space(child->vm);
            }
            proses_hapus(child_pid);
            fork_stats.failed++;
            return PID_INVALID;
        }
    } else {
        child->fd_table = parent->fd_table;
    }

    /* Duplikasi signal handlers */
    if (!(flags & FORK_FLAG_SHARE_SIGNAL)) {
        duplikasi_signal(parent, child);
    }

    /* Duplikasi thread */
    status = duplikasi_thread(parent, child);
    if (status != STATUS_BERHASIL) {
        if (!(flags & FORK_FLAG_SHARE_VM) && child->vm != NULL) {
            vm_destroy_address_space(child->vm);
        }
        proses_hapus(child_pid);
        fork_stats.failed++;
        return PID_INVALID;
    }

    /* Setup context child */
    setup_child_context(child);

    /* Set stack custom jika disediakan */
    if (stack != NULL) {
        context_set_stack(child->main_thread->context, stack);
    }

    /* Tambahkan ke scheduler */
    scheduler_tambah_proses(child);

    /* Tambahkan sebagai child parent */
    proses_tambah_child(parent, child);

    fork_stats.successful++;

    /* Return PID ke parent */
    return child_pid;
}

/*
 * sys_fork
 * --------
 * System call fork.
 *
 * Return: PID child di parent, 0 di child, -1 jika error
 */
pid_t sys_fork(void)
{
    return do_fork(FORK_FLAG_NONE, NULL);
}

/*
 * sys_vfork
 * ---------
 * System call vfork.
 * Parent blocked sampai child exec atau exit.
 *
 * Return: PID child di parent, 0 di child, -1 jika error
 */
pid_t sys_vfork(void)
{
    pid_t result;

    result = do_fork(FORK_FLAG_VFORK | FORK_FLAG_SHARE_VM |
                     FORK_FLAG_SHARE_FILES | FORK_FLAG_SHARE_SIGNAL, NULL);

    if (result > 0) {
        /* vfork: parent blocked sampai child exec/exit */
        proses_t *parent = proses_get_current();
        if (parent != NULL) {
            parent->vfork_child = result;
            scheduler_block(BLOCK_REASON_VFORK);
        }
    }

    fork_stats.vforks++;

    return result;
}

/*
 * sys_clone
 * ---------
 * System call clone (thread creation).
 *
 * Parameter:
 *   flags     - Flag clone
 *   stack     - Stack untuk thread baru
 *   parent_tid - Pointer untuk menyimpan TID child
 *   child_tls  - Thread-local storage untuk child
 *   child_tid  - Pointer untuk menyimpan TID di child
 *
 * Return: TID thread baru, atau -1 jika error
 */
pid_t sys_clone(tak_bertanda32_t flags, void *stack,
                 pid_t *parent_tid, void *child_tls,
                 pid_t *child_tid)
{
    proses_t *parent;
    proses_t *child;
    pid_t new_tid;
    tak_bertanda32_t fork_flags;

    parent = proses_get_current();
    if (parent == NULL) {
        return PID_INVALID;
    }

    /* Tentukan flags fork */
    fork_flags = 0;

    if (flags & CLONE_VM) {
        fork_flags |= FORK_FLAG_SHARE_VM;
    }
    if (flags & CLONE_FILES) {
        fork_flags |= FORK_FLAG_SHARE_FILES;
    }
    if (flags & CLONE_SIGHAND) {
        fork_flags |= FORK_FLAG_SHARE_SIGNAL;
    }

    /* Buat child */
    new_tid = do_fork(fork_flags, stack);
    if (new_tid == PID_INVALID) {
        return PID_INVALID;
    }

    child = proses_cari(new_tid);
    if (child == NULL) {
        return PID_INVALID;
    }

    /* Set parent_tid */
    if (parent_tid != NULL) {
        *parent_tid = new_tid;
    }

    /* Set child_tid di child */
    if (child_tid != NULL) {
        child->set_child_tid = child_tid;
        *child_tid = new_tid;
    }

    /* Set TLS */
    if (child_tls != NULL && child->main_thread != NULL) {
        child->main_thread->tls = child_tls;
    }

    return new_tid;
}

/*
 * fork_get_stats
 * --------------
 * Dapatkan statistik fork.
 *
 * Parameter:
 *   total      - Pointer untuk total fork
 *   successful - Pointer untuk fork berhasil
 *   failed     - Pointer untuk fork gagal
 *   vforks     - Pointer untuk vfork
 */
void fork_get_stats(tak_bertanda64_t *total, tak_bertanda64_t *successful,
                    tak_bertanda64_t *failed, tak_bertanda64_t *vforks)
{
    if (total != NULL) {
        *total = fork_stats.total_forks;
    }

    if (successful != NULL) {
        *successful = fork_stats.successful;
    }

    if (failed != NULL) {
        *failed = fork_stats.failed;
    }

    if (vforks != NULL) {
        *vforks = fork_stats.vforks;
    }
}

/*
 * fork_print_stats
 * ----------------
 * Print statistik fork.
 */
void fork_print_stats(void)
{
    kernel_printf("\n[FORK] Statistik Fork:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Total Forks    : %lu\n", fork_stats.total_forks);
    kernel_printf("  Successful     : %lu\n", fork_stats.successful);
    kernel_printf("  Failed         : %lu\n", fork_stats.failed);
    kernel_printf("  vFork Calls    : %lu\n", fork_stats.vforks);
    kernel_printf("  Success Rate   : %lu%%\n",
                  fork_stats.total_forks > 0 ?
                  (fork_stats.successful * 100 / fork_stats.total_forks) : 0);
    kernel_printf("========================================\n");
}
