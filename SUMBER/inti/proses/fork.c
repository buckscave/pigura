/*
 * PIGURA OS - FORK.C
 * -------------------
 * Implementasi duplikasi proses (fork).
 *
 * Berkas ini berisi fungsi-fungsi untuk menduplikasi proses,
 * membuat salinan address space, dan setup context untuk child.
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

/* Flag duplikasi */
#define FORK_FLAG_NONE          0x00
#define FORK_FLAG_SHARE_VM      0x01   /* Share address space */
#define FORK_FLAG_SHARE_FILES   0x02   /* Share file descriptors */
#define FORK_FLAG_SHARE_SIGNAL  0x04   /* Share signal handlers */
#define FORK_FLAG_VFORK         0x08   /* vfork semantics */
#define FORK_FLAG_SHARE_CWD     0x10   /* Share current directory */
#define FORK_FLAG_CLEAR_SA      0x20   /* Clear signal actions */

/* Return value untuk child */
#define FORK_CHILD_RETURN       0

/* Exit signal default */
#define DEFAULT_EXIT_SIGNAL     SIGCHLD

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
    tak_bertanda64_t clones;
    tak_bertanda64_t pages_copied;
    tak_bertanda64_t time_spent;
} fork_stats = {0, 0, 0, 0, 0, 0, 0};

/* Status inisialisasi */
static bool_t fork_initialized = SALAH;

/* Lock */
static spinlock_t fork_lock;

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
    tak_bertanda64_t start_time;
    
    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    start_time = hal_timer_get_ticks();
    
    /* Buat VM descriptor baru */
    new_vm = vm_create_address_space();
    if (new_vm == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    child->vm = new_vm;
    
    /* Copy memory regions dari parent */
    if (parent->vm != NULL) {
        status_t status;
        
        /* Copy semua memory mapping */
        status = vm_copy_regions(parent->vm, new_vm);
        
        if (status != STATUS_BERHASIL) {
            vm_destroy_address_space(new_vm);
            child->vm = NULL;
            return status;
        }
        
        /* Copy memory statistik */
        child->start_code = parent->start_code;
        child->end_code = parent->end_code;
        child->start_data = parent->start_data;
        child->end_data = parent->end_data;
        child->start_stack = parent->start_stack;
        child->brk_start = parent->brk_start;
        child->brk_current = parent->brk_current;
        child->brk_end = parent->brk_end;
    }
    
    fork_stats.time_spent += (hal_timer_get_ticks() - start_time);
    
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
    for (i = 0; i < FD_MAKS_PER_PROSES; i++) {
        if (parent->fd_table[i] != NULL) {
            /* Duplikasi referensi */
            child->fd_table[i] = parent->fd_table[i];
            
            /* Increment reference count jika ada */
            if (child->fd_table[i]->ops != NULL &&
                child->fd_table[i]->ops->dup != NULL) {
                child->fd_table[i]->ops->dup(child->fd_table[i]);
            } else {
                /* Manual increment */
                child->fd_table[i]->ref_count++;
            }
        }
    }
    
    child->fd_count = parent->fd_count;
    child->fd_next_free = parent->fd_next_free;
    child->fd_max = parent->fd_max;
    
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
    for (i = 1; i <= SIGNAL_JUMLAH; i++) {
        child->signal_handlers[i - 1] = parent->signal_handlers[i - 1];
    }
    
    /* Copy signal mask */
    child->signal_mask = parent->signal_mask;
    
    /* Clear pending signals */
    child->signal_pending = 0;
    child->signal_ignored = parent->signal_ignored;
    
    /* Copy signal trampoline */
    child->signal_trampoline = parent->signal_trampoline;
    child->saved_signal_mask = 0;
    child->signal_count = 0;
    
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
    cpu_context_t *child_ctx;
    void *stack;
    void *kstack;
    ukuran_t stack_size;
    
    if (parent == NULL || child == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    parent_thread = parent->main_thread;
    if (parent_thread == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Alokasikan thread baru */
    child_thread = thread_alokasi();
    if (child_thread == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    /* Copy properties */
    child_thread->pid = child->pid;
    child_thread->tid = child->pid;  /* TID = PID untuk main thread */
    child_thread->proses = child;
    child_thread->status = THREAD_STATUS_SIAP;
    child_thread->prioritas = parent_thread->prioritas;
    child_thread->nice = parent_thread->nice;
    child_thread->quantum = parent_thread->quantum;
    child_thread->flags = parent_thread->flags;
    child_thread->cpu_affinity = parent_thread->cpu_affinity;
    child_thread->last_cpu = parent_thread->last_cpu;
    
    /* Alokasikan user stack baru */
    stack_size = parent_thread->stack_size;
    stack = thread_alokasi_stack(stack_size, BENAR);
    
    if (stack == NULL) {
        thread_bebaskan(child_thread);
        return STATUS_MEMORI_HABIS;
    }
    
    child_thread->stack_base = (void *)((tak_bertanda8_t *)stack - stack_size);
    child_thread->stack_ptr = stack;
    child_thread->stack_size = stack_size;
    
    /* Alokasikan kernel stack */
    kstack = thread_alokasi_kstack();
    if (kstack == NULL) {
        thread_bebaskan_stack(stack, stack_size, BENAR);
        thread_bebaskan(child_thread);
        return STATUS_MEMORI_HABIS;
    }
    
    child_thread->kstack_base = (void *)((tak_bertanda8_t *)kstack -
                                          STACK_UKURAN_KERNEL);
    child_thread->kstack_ptr = kstack;
    child_thread->kstack_size = STACK_UKURAN_KERNEL;
    
    /* Duplikasi context */
    child_ctx = context_dup(parent_thread->context);
    if (child_ctx == NULL) {
        thread_bebaskan_kstack(kstack);
        thread_bebaskan_stack(stack, stack_size, BENAR);
        thread_bebaskan(child_thread);
        return STATUS_MEMORI_HABIS;
    }
    
    /* Set return value 0 untuk child */
    context_set_return(child_ctx, FORK_CHILD_RETURN);
    
    /* Set stack di context */
    context_set_stack(child_ctx, stack);
    
    /* Set CR3 untuk child */
    if (child->vm != NULL) {
        context_set_cr3(child_ctx, child->vm->cr3);
    }
    
    child_thread->context = child_ctx;
    child_thread->start_time = hal_timer_get_ticks();
    child_thread->cpu_time = 0;
    
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
    thread->status = THREAD_STATUS_SIAP;
    child->status = PROSES_STATUS_SIAP;
    
    /* Set return value 0 untuk child */
    if (thread->context != NULL) {
        context_set_return(thread->context, 0);
    }
    
    return STATUS_BERHASIL;
}

/*
 * reparent_to_init
 * ----------------
 * Reparent proses ke init (PID 1).
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
static void reparent_to_init(proses_t *proses)
    __attribute__((unused));
static void reparent_to_init(proses_t *proses)
{
    proses_t *init;
    
    if (proses == NULL) {
        return;
    }
    
    init = proses_cari(PID_INIT);
    if (init == NULL) {
        init = proses_dapat_kernel();
    }
    
    if (init != NULL && init != proses->parent) {
        /* Hapus dari parent lama */
        if (proses->parent != NULL) {
            proses_hapus_child(proses->parent, proses);
        }
        
        /* Tambah ke init */
        proses->ppid = init->pid;
        proses_tambah_child(init, proses);
    }
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
    spinlock_init(&fork_lock);
    
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
 * Return: PID child di parent, 0 di child, PID_TIDAK_ADA jika error
 */
pid_t do_fork(tak_bertanda32_t flags, void *stack)
{
    proses_t *parent;
    proses_t *child;
    pid_t child_pid;
    status_t status;
    tak_bertanda64_t start_time;
    
    if (!fork_initialized) {
        return PID_TIDAK_ADA;
    }
    
    start_time = hal_timer_get_ticks();
    
    parent = proses_dapat_saat_ini();
    if (parent == NULL) {
        return PID_TIDAK_ADA;
    }
    
    /* Tidak boleh fork kernel process kecuali dengan flag khusus */
    if ((parent->flags & PROSES_FLAG_KERNEL) &&
        !(flags & FORK_FLAG_SHARE_VM)) {
        return PID_TIDAK_ADA;
    }
    
    fork_stats.total_forks++;
    
    /* Alokasikan struktur proses child */
    child_pid = proses_buat(parent->nama, parent->pid, 0);
    if (child_pid == PID_TIDAK_ADA) {
        fork_stats.failed++;
        return PID_TIDAK_ADA;
    }
    
    child = proses_cari(child_pid);
    if (child == NULL) {
        fork_stats.failed++;
        return PID_TIDAK_ADA;
    }
    
    spinlock_kunci(&fork_lock);
    
    /* Copy properties dari parent */
    child->gid = parent->gid;
    child->uid = parent->uid;
    child->euid = parent->euid;
    child->egid = parent->egid;
    child->suid = parent->suid;
    child->sgid = parent->sgid;
    child->fsuid = parent->fsuid;
    child->fsgid = parent->fsgid;
    child->pgid = parent->pgid;
    child->sid = parent->sid;
    child->prioritas = parent->prioritas;
    child->nice = parent->nice;
    child->exit_signal = DEFAULT_EXIT_SIGNAL;
    
    /* Copy groups */
    child->ngroups = parent->ngroups;
    kernel_memcpy(child->groups, parent->groups,
                  sizeof(gid_t) * parent->ngroups);
    
    /* Copy capabilities */
    child->cap_effective = parent->cap_effective;
    child->cap_permitted = parent->cap_permitted;
    child->cap_inheritable = parent->cap_inheritable;
    
    /* Copy resource limits */
    child->limit_stack = parent->limit_stack;
    child->limit_data = parent->limit_data;
    child->limit_core = parent->limit_core;
    child->limit_memory = parent->limit_memory;
    child->limit_cpu = parent->limit_cpu;
    child->limit_nofile = parent->limit_nofile;
    child->limit_nproc = parent->limit_nproc;
    
    kernel_strncpy(child->cwd, parent->cwd, PATH_MAX - 1);
    kernel_strncpy(child->root, parent->root, PATH_MAX - 1);
    kernel_strncpy(child->exe, parent->exe, PATH_MAX - 1);
    
    /* Duplikasi address space */
    if (!(flags & FORK_FLAG_SHARE_VM)) {
        status = duplikasi_vm(parent, child);
        if (status != STATUS_BERHASIL) {
            spinlock_buka(&fork_lock);
            proses_hapus(child_pid);
            fork_stats.failed++;
            return PID_TIDAK_ADA;
        }
    } else {
        child->vm = parent->vm;
        child->start_code = parent->start_code;
        child->end_code = parent->end_code;
        child->start_data = parent->start_data;
        child->end_data = parent->end_data;
        child->start_stack = parent->start_stack;
        child->brk_start = parent->brk_start;
        child->brk_current = parent->brk_current;
        child->brk_end = parent->brk_end;
    }
    
    /* Duplikasi file descriptors */
    if (!(flags & FORK_FLAG_SHARE_FILES)) {
        status = duplikasi_fd(parent, child);
        if (status != STATUS_BERHASIL) {
            if (!(flags & FORK_FLAG_SHARE_VM) && child->vm != NULL) {
                vm_destroy_address_space(child->vm);
            }
            spinlock_buka(&fork_lock);
            proses_hapus(child_pid);
            fork_stats.failed++;
            return PID_TIDAK_ADA;
        }
    } else {
        kernel_memcpy(child->fd_table, parent->fd_table,
                      sizeof(parent->fd_table));
        child->fd_count = parent->fd_count;
    }
    
    /* Duplikasi signal handlers */
    if (!(flags & FORK_FLAG_SHARE_SIGNAL)) {
        duplikasi_signal(parent, child);
    } else {
        kernel_memcpy(child->signal_handlers, parent->signal_handlers,
                      sizeof(parent->signal_handlers));
        child->signal_mask = parent->signal_mask;
    }
    
    /* Duplikasi thread */
    status = duplikasi_thread(parent, child);
    if (status != STATUS_BERHASIL) {
        if (!(flags & FORK_FLAG_SHARE_VM) && child->vm != NULL) {
            vm_destroy_address_space(child->vm);
        }
        spinlock_buka(&fork_lock);
        proses_hapus(child_pid);
        fork_stats.failed++;
        return PID_TIDAK_ADA;
    }
    
    /* Setup context child */
    setup_child_context(child);
    
    /* Set stack custom jika disediakan */
    if (stack != NULL && child->main_thread != NULL &&
        child->main_thread->context != NULL) {
        context_set_stack(child->main_thread->context, stack);
    }
    
    /* Set flags */
    if (flags & FORK_FLAG_VFORK) {
        child->flags |= PROSES_FLAG_VFORK;
    }
    
    /* Tambahkan ke scheduler */
    scheduler_tambah_thread(child->main_thread);
    
    fork_stats.successful++;
    fork_stats.time_spent += (hal_timer_get_ticks() - start_time);
    
    spinlock_buka(&fork_lock);
    
    /* Handle vfork blocking */
    if (flags & FORK_FLAG_VFORK) {
        fork_stats.vforks++;
        
        /* Parent blocked sampai child exec atau exit */
        parent->vfork_child = child_pid;
        scheduler_block(BLOCK_ALASAN_VFORK);
    }
    
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
    
    fork_stats.vforks++;
    
    return result;
}

/*
 * sys_clone
 * ---------
 * System call clone (thread creation).
 *
 * Parameter:
 *   flags      - Flag clone
 *   stack      - Stack untuk thread baru
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
    
    parent = proses_dapat_saat_ini();
    if (parent == NULL) {
        return PID_TIDAK_ADA;
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
    if (flags & CLONE_VFORK) {
        fork_flags |= FORK_FLAG_VFORK;
    }
    
    /* Buat child */
    new_tid = do_fork(fork_flags, stack);
    if (new_tid == PID_TIDAK_ADA) {
        return PID_TIDAK_ADA;
    }
    
    child = proses_cari(new_tid);
    if (child == NULL) {
        return PID_TIDAK_ADA;
    }
    
    /* Set parent_tid */
    if (parent_tid != NULL) {
        if (validasi_pointer_user(parent_tid)) {
            *parent_tid = new_tid;
        }
    }
    
    /* Set child_tid di child */
    if (child_tid != NULL) {
        if (validasi_pointer_user(child_tid)) {
            child->set_child_tid = child_tid;
            *child_tid = new_tid;
        }
    }
    
    /* Set TLS */
    if (child_tls != NULL && child->main_thread != NULL) {
        child->main_thread->tls = child_tls;
    }
    
    fork_stats.clones++;
    
    return new_tid;
}

/*
 * fork_dapat_statistik
 * --------------------
 * Dapatkan statistik fork.
 *
 * Parameter:
 *   total      - Pointer untuk total fork
 *   successful - Pointer untuk fork berhasil
 *   failed     - Pointer untuk fork gagal
 *   vforks     - Pointer untuk vfork
 */
void fork_dapat_statistik(tak_bertanda64_t *total,
                          tak_bertanda64_t *successful,
                          tak_bertanda64_t *failed,
                          tak_bertanda64_t *vforks)
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
    kernel_printf("  Clone Calls    : %lu\n", fork_stats.clones);
    kernel_printf("  Pages Copied   : %lu\n", fork_stats.pages_copied);
    kernel_printf("  Time Spent     : %lu ticks\n", fork_stats.time_spent);
    kernel_printf("  Success Rate   : %lu%%\n",
                  fork_stats.total_forks > 0 ?
                  (fork_stats.successful * 100 / fork_stats.total_forks) : 0);
    kernel_printf("========================================\n");
}
