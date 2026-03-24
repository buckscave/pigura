/*
 * PIGURA OS - EXIT.C
 * -------------------
 * Implementasi terminal proses.
 *
 * Berkas ini berisi fungsi-fungsi untuk menghentikan proses
 * dan membersihkan resource yang digunakan.
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

/* Exit code mask */
#define EXIT_CODE_MASK          0xFF

/* Maximum zombie age (ticks) before warning */
#define ZOMBIE_MAX_AGE           10000

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik exit */
static struct {
    tak_bertanda64_t total_exits;
    tak_bertanda64_t normal_exits;
    tak_bertanda64_t signal_exits;
    tak_bertanda64_t kernel_exits;
    tak_bertanda64_t zombie_cleanups;
} exit_stats = {0, 0, 0, 0, 0};

/* Status inisialisasi */
static bool_t exit_initialized = SALAH;

/* Proses yang sedang dalam status zombie */
static proses_t *zombie_list = NULL;
static spinlock_t zombie_lock;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * cleanup_file_descriptors
 * ------------------------
 * Bersihkan file descriptors proses.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
static void cleanup_file_descriptors(proses_t *proses)
{
    tak_bertanda32_t i;
    
    if (proses == NULL) {
        return;
    }
    
    for (i = 0; i < FD_MAKS_PER_PROSES; i++) {
        if (proses->fd_table[i] != NULL) {
            /* Tutup file descriptor - bebaskan memori */
            /* TODO: Implementasikan berkas_tutup dari VFS */
            kfree(proses->fd_table[i]);
            proses->fd_table[i] = NULL;
        }
    }
    
    proses->fd_count = 0;
}

/*
 * cleanup_memory
 * --------------
 * Bersihkan memory proses.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
static void cleanup_memory(proses_t *proses)
{
    thread_t *thread;
    thread_t *next_thread;
    
    if (proses == NULL) {
        return;
    }
    
    /* Hapus address space */
    if (proses->vm != NULL) {
        vm_destroy_address_space(proses->vm);
        proses->vm = NULL;
    }
    
    /* Hapus threads */
    thread = proses->threads;
    while (thread != NULL) {
        next_thread = thread->next;
        
        /* Hapus context */
        if (thread->context != NULL) {
            context_hancurkan(thread->context);
            thread->context = NULL;
        }
        
        /* Hapus stack */
        if (thread->stack_base != NULL) {
            thread_bebaskan_stack(thread->stack_ptr,
                                   thread->stack_size,
                                   !(proses->flags & PROSES_FLAG_KERNEL));
            thread->stack_base = NULL;
            thread->stack_ptr = NULL;
        }
        
        /* Hapus kernel stack */
        if (thread->kstack_base != NULL) {
            thread_bebaskan_kstack(thread->kstack_ptr);
            thread->kstack_base = NULL;
            thread->kstack_ptr = NULL;
        }
        
        /* Bebaskan struktur thread */
        thread_bebaskan(thread);
        
        thread = next_thread;
    }
    
    proses->threads = NULL;
    proses->main_thread = NULL;
    proses->thread_count = 0;
}

/*
 * reparent_children
 * -----------------
 * Pindahkan child proses ke init.
 *
 * Parameter:
 *   proses - Pointer ke proses yang exit
 */
static void reparent_children(proses_t *proses)
{
    proses_t *child;
    proses_t *init;
    proses_t *next;
    
    if (proses == NULL) {
        return;
    }
    
    init = proses_cari(PID_INIT);
    if (init == NULL) {
        init = proses_dapat_kernel();
    }
    
    child = proses->children;
    while (child != NULL) {
        next = child->sibling;
        
        /* Pindahkan ke init */
        proses_hapus_child(proses, child);
        child->ppid = init->pid;
        proses_tambah_child(init, child);
        
        /* Kirim SIGCHLD ke init */
        signal_kirim_ke_proses(init, SIGCHLD);
        
        /* Jika child zombie, notify init */
        if (child->status == PROSES_STATUS_ZOMBIE) {
            init->zombie_children++;
        }
        
        child = next;
    }
    
    proses->children = NULL;
}

/*
 * notify_parent
 * -------------
 * Beritahu parent tentang exit.
 *
 * Parameter:
 *   proses - Pointer ke proses yang exit
 */
static void notify_parent(proses_t *proses)
{
    proses_t *parent;
    
    if (proses == NULL) {
        return;
    }
    
    parent = proses_cari(proses->ppid);
    if (parent == NULL) {
        return;
    }
    
    /* Increment child zombie count */
    parent->zombie_children++;
    
    /* Jika parent menunggu, wake up */
    if (parent->main_thread != NULL &&
        parent->main_thread->status == THREAD_STATUS_TUNGGU) {
        scheduler_unblock(parent->main_thread);
    }
    
    /* Kirim SIGCHLD */
    signal_kirim_ke_proses(parent, SIGCHLD);
}

/*
 * add_to_zombie_list
 * ------------------
 * Tambahkan proses ke zombie list.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
static void add_to_zombie_list(proses_t *proses)
{
    spinlock_kunci(&zombie_lock);
    
    proses->next_zombie = zombie_list;
    zombie_list = proses;
    
    spinlock_buka(&zombie_lock);
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * exit_init
 * ---------
 * Inisialisasi subsistem exit.
 *
 * Return: Status operasi
 */
status_t exit_init(void)
{
    if (exit_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&exit_stats, 0, sizeof(exit_stats));
    spinlock_init(&zombie_lock);
    zombie_list = NULL;
    
    exit_initialized = BENAR;
    
    kernel_printf("[EXIT] Exit subsystem initialized\n");
    
    return STATUS_BERHASIL;
}

/*
 * do_exit
 * -------
 * Exit proses saat ini.
 *
 * Parameter:
 *   exit_code - Exit code
 */
void do_exit(tanda32_t exit_code)
{
    proses_t *proses;
    proses_t *parent;
    
    if (!exit_initialized) {
        PANIC("exit: tidak diinisialisasi");
    }
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        PANIC("exit: tidak ada proses current");
    }
    
    /* Tidak boleh exit kernel process */
    if (proses->flags & PROSES_FLAG_KERNEL) {
        PANIC("exit: tidak dapat exit kernel process");
    }
    
    exit_stats.total_exits++;
    
    /* Set exit code dan status */
    proses->exit_code = exit_code;
    proses->status = PROSES_STATUS_ZOMBIE;
    proses->exit_time = hal_timer_get_ticks();
    
    /* Cek apakah exit karena signal */
    if (exit_code & EXIT_SIGNAL_CORE) {
        exit_stats.signal_exits++;
    } else {
        exit_stats.normal_exits++;
    }
    
    /* Cleanup resources */
    cleanup_file_descriptors(proses);
    cleanup_memory(proses);
    
    /* Reparent children */
    reparent_children(proses);
    
    /* Hapus dari scheduler */
    if (proses->main_thread != NULL) {
        scheduler_hapus_thread(proses->main_thread);
    }
    
    /* Hapus dari list proses aktif */
    proses_hapus_dari_list(proses);
    
    /* Notify parent */
    notify_parent(proses);
    
    /* Tambahkan ke zombie list */
    add_to_zombie_list(proses);
    
    /* Jika parent dalam vfork, unblock */
    parent = proses_cari(proses->ppid);
    if (parent != NULL && parent->vfork_child == proses->pid) {
        parent->vfork_child = 0;
        if (parent->main_thread != NULL) {
            scheduler_unblock(parent->main_thread);
        }
    }
    
    /* Schedule ke proses lain */
    scheduler_schedule();
    
    /* Tidak seharusnya sampai sini */
    PANIC("exit: should not reach here");
}

/*
 * do_exit_group
 * -------------
 * Exit semua thread dalam proses.
 *
 * Parameter:
 *   exit_code - Exit code
 */
void do_exit_group(tanda32_t exit_code)
{
    proses_t *proses;
    thread_t *thread;
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return;
    }
    
    /* Set status zombie untuk semua thread */
    thread = proses->threads;
    while (thread != NULL) {
        thread->status = THREAD_STATUS_ZOMBIE;
        thread = thread->next;
    }
    
    /* Exit proses */
    do_exit(exit_code);
}

/*
 * sys_exit
 * --------
 * System call exit.
 *
 * Parameter:
 *   exit_code - Exit code
 */
void sys_exit(tanda32_t exit_code)
{
    do_exit(exit_code & EXIT_CODE_MASK);
}

/*
 * sys_exit_group
 * --------------
 * System call exit_group.
 *
 * Parameter:
 *   exit_code - Exit code
 */
void sys_exit_group(tanda32_t exit_code)
{
    do_exit_group(exit_code & EXIT_CODE_MASK);
}

/*
 * exit_by_signal
 * --------------
 * Exit proses karena signal.
 *
 * Parameter:
 *   proses  - Pointer ke proses
 *   signal  - Nomor signal
 *   core    - Apakah menghasilkan core dump
 */
void exit_by_signal(proses_t *proses, tak_bertanda32_t signal, bool_t core)
{
    tanda32_t exit_code;
    
    if (proses == NULL) {
        return;
    }
    
    /* Encode exit code */
    exit_code = (signal << EXIT_SIGNAL_SHIFT);
    if (core) {
        exit_code |= EXIT_SIGNAL_CORE;
    }
    
    /* Set sebagai current jika bukan */
    if (proses_dapat_saat_ini() != proses) {
        proses_set_saat_ini(proses);
    }
    
    do_exit(exit_code);
}

/*
 * exit_dapat_statistik
 * --------------------
 * Dapatkan statistik exit.
 *
 * Parameter:
 *   total   - Pointer untuk total exit
 *   normal  - Pointer untuk exit normal
 *   signal  - Pointer untuk exit signal
 *   kernel  - Pointer untuk kernel exit
 */
void exit_dapat_statistik(tak_bertanda64_t *total,
                          tak_bertanda64_t *normal,
                          tak_bertanda64_t *signal,
                          tak_bertanda64_t *kernel)
{
    if (total != NULL) {
        *total = exit_stats.total_exits;
    }
    
    if (normal != NULL) {
        *normal = exit_stats.normal_exits;
    }
    
    if (signal != NULL) {
        *signal = exit_stats.signal_exits;
    }
    
    if (kernel != NULL) {
        *kernel = exit_stats.kernel_exits;
    }
}

/*
 * exit_print_stats
 * ----------------
 * Print statistik exit.
 */
void exit_print_stats(void)
{
    kernel_printf("\n[EXIT] Statistik Exit:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Total Exits    : %lu\n", exit_stats.total_exits);
    kernel_printf("  Normal Exits   : %lu\n", exit_stats.normal_exits);
    kernel_printf("  Signal Exits   : %lu\n", exit_stats.signal_exits);
    kernel_printf("  Kernel Exits   : %lu\n", exit_stats.kernel_exits);
    kernel_printf("  Cleanups       : %lu\n", exit_stats.zombie_cleanups);
    kernel_printf("========================================\n");
}

/*
 * exit_cleanup_zombies
 * --------------------
 * Cleanup zombie processes.
 *
 * Parameter:
 *   max_count - Maksimum zombie untuk di-cleanup (0 = semua)
 *
 * Return: Jumlah zombie yang di-cleanup
 */
tak_bertanda32_t exit_cleanup_zombies(tak_bertanda32_t max_count)
{
    proses_t *proses;
    proses_t *prev;
    proses_t *next;
    tak_bertanda32_t count = 0;
    
    spinlock_kunci(&zombie_lock);
    
    prev = NULL;
    proses = zombie_list;
    
    while (proses != NULL && (max_count == 0 || count < max_count)) {
        next = proses->next_zombie;
        
        /* Cek apakah parent sudah wait */
        if (proses->wait_collected) {
            /* Hapus dari zombie list */
            if (prev == NULL) {
                zombie_list = next;
            } else {
                prev->next_zombie = next;
            }
            
            /* Bebaskan struktur proses */
            proses_bebaskan(proses);
            count++;
            
            exit_stats.zombie_cleanups++;
        } else {
            prev = proses;
        }
        
        proses = next;
    }
    
    spinlock_buka(&zombie_lock);
    
    return count;
}

/*
 * exit_dapat_jumlah_zombie
 * ------------------------
 * Dapatkan jumlah zombie processes.
 *
 * Return: Jumlah zombie
 */
tak_bertanda32_t exit_dapat_jumlah_zombie(void)
{
    proses_t *proses;
    tak_bertanda32_t count = 0;
    
    spinlock_kunci(&zombie_lock);
    
    proses = zombie_list;
    while (proses != NULL) {
        count++;
        proses = proses->next_zombie;
    }
    
    spinlock_buka(&zombie_lock);
    
    return count;
}
