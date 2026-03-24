/*
 * PIGURA OS - THREAD.C
 * ---------------------
 * Implementasi manajemen thread dalam proses.
 *
 * Berkas ini berisi fungsi-fungsi untuk membuat, mengelola, dan
 * menghancurkan thread dalam konteks proses Pigura OS.
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

/* Ukuran stack default */
#define STACK_UKURAN_KERNEL     UKURAN_STACK_KERNEL
#define STACK_UKURAN_USER       UKURAN_STACK_USER

/* Alignment stack */
#define STACK_ALIGNMENT         16

/* Jumlah maksimum thread yang menunggu */
#define MAX_WAITING_THREADS      16

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik thread */
static struct {
    tak_bertanda64_t created;
    tak_bertanda64_t exited;
    tak_bertanda64_t active;
    tak_bertanda64_t joins;
    tak_bertanda64_t detaches;
    tak_bertanda64_t allocations;
    tak_bertanda64_t frees;
} thread_stats = {0, 0, 0, 0, 0, 0, 0};

/* Status inisialisasi */
static bool_t thread_initialized = SALAH;

/* TID berikutnya */
static tak_bertanda32_t next_tid = 1;

/* Lock */
static spinlock_t thread_lock;

/*
 * ============================================================================
 * FUNGSI INTERNAL STACK (INTERNAL STACK FUNCTIONS)
 * ============================================================================
 */

/*
 * thread_alloc_stack_internal
 * ---------------------------
 * Alokasikan stack untuk thread.
 *
 * Parameter:
 *   size    - Ukuran stack (0 untuk default)
 *   is_user - Apakah stack untuk user mode
 *
 * Return: Pointer ke top of stack, atau NULL
 */
static void *thread_alloc_stack_internal(ukuran_t size, bool_t is_user)
{
    void *stack_mem;
    ukuran_t actual_size;
    alamat_virtual_t addr;
    
    /* Gunakan ukuran default jika tidak ditentukan */
    if (size == 0) {
        size = is_user ? STACK_UKURAN_USER : STACK_UKURAN_KERNEL;
    }
    
    /* Align ukuran ke halaman */
    actual_size = RATAKAN_ATAS(size, UKURAN_HALAMAN);
    
    if (is_user) {
        /* Untuk user thread, alokasi melalui VM proses */
        proses_t *proses;
        
        proses = proses_dapat_saat_ini();
        if (proses == NULL || proses->vm == NULL) {
            return NULL;
        }
        
        /* Alokasikan region di user space */
        addr = vm_alloc(proses->vm, actual_size,
                        VMA_FLAG_READ | VMA_FLAG_WRITE | VMA_FLAG_STACK,
                        ALAMAT_STACK_USER);
        if (addr == 0) {
            return NULL;
        }
        
        /* Stack grows down, return top */
        return (void *)(addr + actual_size);
    } else {
        /* Untuk kernel thread */
        stack_mem = kmalloc(actual_size);
        if (stack_mem == NULL) {
            return NULL;
        }
        
        /* Clear stack untuk keamanan */
        kernel_memset(stack_mem, 0, actual_size);
        
        /* Return top of stack */
        return (void *)((tak_bertanda8_t *)stack_mem + actual_size);
    }
}

/*
 * thread_free_stack_internal
 * --------------------------
 * Bebaskan stack thread.
 *
 * Parameter:
 *   stack   - Pointer ke stack
 *   size    - Ukuran stack
 *   is_user - Apakah stack user mode
 */
static void thread_free_stack_internal(void *stack, ukuran_t size,
                                        bool_t is_user)
{
    ukuran_t actual_size;
    void *stack_base;
    
    if (stack == NULL) {
        return;
    }
    
    if (size == 0) {
        size = is_user ? STACK_UKURAN_USER : STACK_UKURAN_KERNEL;
    }
    
    actual_size = RATAKAN_ATAS(size, UKURAN_HALAMAN);
    
    if (is_user) {
        /* Untuk user thread, bebaskan melalui VM */
        proses_t *proses;
        alamat_virtual_t base;
        
        proses = proses_dapat_saat_ini();
        if (proses != NULL && proses->vm != NULL) {
            base = (alamat_virtual_t)stack - actual_size;
            vm_free(proses->vm, base, actual_size);
        }
    } else {
        /* Untuk kernel thread */
        stack_base = (void *)((tak_bertanda8_t *)stack - actual_size);
        kfree(stack_base);
    }
}

/*
 * thread_entry_wrapper
 * --------------------
 * Wrapper untuk entry point thread.
 * Memastikan thread exit dengan benar setelah selesai.
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
    
    /* Thread exit otomatis */
    thread_exit(NULL);
}

/*
 * ============================================================================
 * FUNGSI PUBLIK THREAD (PUBLIC THREAD FUNCTIONS)
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
    
    /* Reset statistik */
    kernel_memset(&thread_stats, 0, sizeof(thread_stats));
    
    /* Init lock */
    spinlock_init(&thread_lock);
    
    /* Reset TID counter */
    next_tid = 1;
    
    thread_initialized = BENAR;
    
    kernel_printf("[THREAD] Thread manager initialized\n");
    
    return STATUS_BERHASIL;
}

/*
 * thread_buat
 * -----------
 * Buat thread baru dalam proses.
 *
 * Parameter:
 *   pid   - Process ID
 *   entry - Entry point thread
 *   arg   - Argumen thread
 *   flags - Flag thread
 *
 * Return: Thread ID, atau TID_TIDAK_ADA jika gagal
 */
tid_t thread_buat(pid_t pid, void (*entry)(void *), void *arg,
                  tak_bertanda32_t flags)
{
    proses_t *proses;
    thread_t *thread;
    void *stack;
    void *kstack;
    ukuran_t stack_size;
    cpu_context_t *ctx;
    bool_t is_kernel;
    
    if (!thread_initialized || entry == NULL) {
        return TID_TIDAK_ADA;
    }
    
    /* Cari proses parent */
    proses = proses_cari(pid);
    if (proses == NULL) {
        return TID_TIDAK_ADA;
    }
    
    /* Cek batas thread */
    if (proses->thread_count >= proses->thread_limit) {
        return TID_TIDAK_ADA;
    }
    
    /* Tentukan tipe thread */
    is_kernel = (flags & THREAD_FLAG_KERNEL) ? BENAR : SALAH;
    
    /* Alokasikan struktur thread */
    thread = thread_alokasi();
    if (thread == NULL) {
        return TID_TIDAK_ADA;
    }
    
    spinlock_kunci(&thread_lock);
    
    /* Set properties */
    thread->pid = pid;
    thread->proses = proses;
    thread->flags = flags;
    thread->prioritas = proses->prioritas;
    
    /* Alokasikan stack user */
    stack_size = is_kernel ? STACK_UKURAN_KERNEL : STACK_UKURAN_USER;
    stack = thread_alloc_stack_internal(stack_size, !is_kernel);
    
    if (stack == NULL) {
        thread_bebaskan(thread);
        spinlock_buka(&thread_lock);
        return TID_TIDAK_ADA;
    }
    
    thread->stack_base = (void *)((tak_bertanda8_t *)stack - stack_size);
    thread->stack_ptr = stack;
    thread->stack_size = stack_size;
    
    /* Alokasikan kernel stack untuk user thread */
    if (!is_kernel) {
        kstack = kmalloc(STACK_UKURAN_KERNEL);
        if (kstack == NULL) {
            thread_free_stack_internal(stack, stack_size, SALAH);
            thread_bebaskan(thread);
            spinlock_buka(&thread_lock);
            return TID_TIDAK_ADA;
        }
        
        thread->kstack_base = kstack;
        thread->kstack_ptr = (void *)((tak_bertanda8_t *)kstack +
                                      STACK_UKURAN_KERNEL);
        thread->kstack_size = STACK_UKURAN_KERNEL;
    }
    
    /* Buat context */
    ctx = context_buat((alamat_virtual_t)thread_entry_wrapper,
                       stack, !is_kernel);
    
    if (ctx == NULL) {
        if (thread->kstack_base != NULL) {
            kfree(thread->kstack_base);
        }
        thread_free_stack_internal(stack, stack_size, !is_kernel);
        thread_bebaskan(thread);
        spinlock_buka(&thread_lock);
        return TID_TIDAK_ADA;
    }
    
    /* Set CR3 jika ada */
    if (proses->vm != NULL) {
        context_set_cr3(ctx, proses->vm->cr3);
    }
    
    thread->context = ctx;
    thread->status = THREAD_STATUS_SIAP;
    thread->quantum = dapatkan_quantum(thread->prioritas);
    thread->start_time = hal_timer_get_ticks();
    
    /* Setup argumen di stack */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    {
        tak_bertanda64_t *sp = (tak_bertanda64_t *)stack;
        sp -= 2;
        sp[0] = (tak_bertanda64_t)(alamat_ptr_t)arg;
        sp[1] = (tak_bertanda64_t)(alamat_ptr_t)entry;
        context_set_stack(ctx, sp);
    }
#else
    {
        tak_bertanda32_t *sp = (tak_bertanda32_t *)stack;
        sp -= 2;
        sp[0] = (tak_bertanda32_t)(alamat_ptr_t)arg;
        sp[1] = (tak_bertanda32_t)(alamat_ptr_t)entry;
        context_set_stack(ctx, sp);
    }
#endif
    
    /* Tambahkan ke list proses */
    thread->next = proses->threads;
    if (proses->threads != NULL) {
        proses->threads->prev = thread;
    }
    proses->threads = thread;
    proses->thread_count++;
    
    /* Update statistik */
    thread_stats.created++;
    thread_stats.active++;
    thread_stats.allocations++;
    
    spinlock_buka(&thread_lock);
    
    /* Tambahkan ke scheduler */
    scheduler_tambah_thread(thread);
    
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
    thread_t *thread;
    proses_t *proses;
    
    if (!thread_initialized) {
        return;
    }
    
    thread = scheduler_dapat_current();
    if (thread == NULL) {
        return;
    }
    
    proses = thread->proses;
    
    spinlock_kunci(&thread_lock);
    
    /* Set retval dan status */
    thread->retval = retval;
    thread->status = THREAD_STATUS_ZOMBIE;
    thread->exit_code = 0;
    
    /* Jika ada thread yang menunggu join, wake up */
    if (thread->waiting != NULL) {
        scheduler_unblock(thread->waiting);
        thread->waiting = NULL;
    }
    
    /* Update statistik */
    thread_stats.exited++;
    if (thread_stats.active > 0) {
        thread_stats.active--;
    }
    
    /* Hapus dari scheduler */
    scheduler_hapus_thread(thread);
    
    /* Jika main thread, exit proses */
    if (thread->flags & THREAD_FLAG_MAIN) {
        if (proses != NULL) {
            proses_exit(proses->pid, retval != NULL ?
                        *(tanda32_t *)retval : 0);
        }
    }
    
    spinlock_buka(&thread_lock);
    
    /* Schedule ke thread lain */
    scheduler_schedule();
    
    /* Tidak seharusnya sampai sini */
    kernel_panic(__FILE__, __LINE__, "thread_exit: should not reach here");
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
status_t thread_join(tid_t tid, void **retval)
{
    thread_t *thread;
    thread_t *current;
    proses_t *proses;
    
    if (!thread_initialized) {
        return STATUS_GAGAL;
    }
    
    proses = proses_dapat_saat_ini();
    if (proses == NULL) {
        return STATUS_GAGAL;
    }
    
    current = scheduler_dapat_current();
    if (current == NULL) {
        return STATUS_GAGAL;
    }
    
    /* Cari thread */
    thread = thread_cari(tid);
    if (thread == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Tidak bisa join diri sendiri */
    if (thread == current) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Cek jika sudah detached */
    if (thread->flags & THREAD_FLAG_DETACHED) {
        return STATUS_GAGAL;
    }
    
    /* Cek jika sudah di-join */
    if (thread->flags & THREAD_FLAG_JOINED) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Cek jika sudah selesai */
    if (thread->status == THREAD_STATUS_ZOMBIE) {
        if (retval != NULL) {
            *retval = thread->retval;
        }
        
        /* Cleanup zombie thread */
        thread_bebaskan(thread);
        return STATUS_BERHASIL;
    }
    
    /* Tandai bahwa kita menunggu */
    thread->waiting = current;
    thread->flags |= THREAD_FLAG_JOINED;
    
    thread_stats.joins++;
    
    /* Block sampai thread selesai */
    scheduler_block(BLOCK_ALASAN_WAIT);
    
    /* Thread sudah selesai */
    if (retval != NULL) {
        *retval = thread->retval;
    }
    
    /* Cleanup */
    thread_bebaskan(thread);
    
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
status_t thread_detach(tid_t tid)
{
    thread_t *thread;
    
    if (!thread_initialized) {
        return STATUS_GAGAL;
    }
    
    thread = thread_cari(tid);
    if (thread == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Tidak bisa detach jika sudah di-join */
    if (thread->flags & THREAD_FLAG_JOINED) {
        return STATUS_GAGAL;
    }
    
    /* Set flag detached */
    thread->flags |= THREAD_FLAG_DETACHED;
    
    thread_stats.detaches++;
    
    /* Jika thread sudah zombie, langsung cleanup */
    if (thread->status == THREAD_STATUS_ZOMBIE) {
        thread_bebaskan(thread);
    }
    
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
tid_t thread_self(void)
{
    thread_t *thread;
    
    thread = scheduler_dapat_current();
    if (thread == NULL) {
        return TID_TIDAK_ADA;
    }
    
    return thread->tid;
}

/*
 * thread_cancel
 * -------------
 * Cancel thread.
 *
 * Parameter:
 *   tid - Thread ID
 *
 * Return: Status operasi
 */
status_t thread_cancel(tid_t tid)
{
    thread_t *thread;
    thread_t *current;
    
    if (!thread_initialized) {
        return STATUS_GAGAL;
    }
    
    thread = thread_cari(tid);
    if (thread == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    current = scheduler_dapat_current();
    
    /* Tidak bisa cancel diri sendiri */
    if (thread == current) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Tidak bisa cancel thread kernel */
    if (thread->flags & THREAD_FLAG_KERNEL) {
        return STATUS_AKSES_DITOLAK;
    }
    
    /* Set flag cancel */
    thread->flags |= THREAD_FLAG_SIGNAL;
    
    /* Kirim signal cancel */
    if (thread->proses != NULL) {
        signal_kirim_ke_proses(thread->proses, SIGNAL_TERM);
    }
    
    return STATUS_BERHASIL;
}

/*
 * thread_set_prioritas
 * --------------------
 * Set prioritas thread.
 *
 * Parameter:
 *   tid       - Thread ID
 *   prioritas - Prioritas baru
 *
 * Return: Status operasi
 */
status_t thread_set_prioritas(tid_t tid, tak_bertanda32_t prioritas)
{
    thread_t *thread;
    
    if (!thread_initialized) {
        return STATUS_GAGAL;
    }
    
    if (prioritas > PRIORITAS_KRITIS) {
        return STATUS_PARAM_INVALID;
    }
    
    thread = thread_cari(tid);
    if (thread == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    return scheduler_set_prioritas(thread, prioritas);
}

/*
 * thread_dapat_prioritas
 * ----------------------
 * Dapatkan prioritas thread.
 *
 * Parameter:
 *   tid - Thread ID
 *
 * Return: Prioritas thread
 */
tak_bertanda32_t thread_dapat_prioritas(tid_t tid)
{
    thread_t *thread;
    
    thread = thread_cari(tid);
    if (thread == NULL) {
        return PRIORITAS_NORMAL;
    }
    
    return thread->prioritas;
}

/*
 * thread_cari
 * -----------
 * Cari thread berdasarkan TID.
 *
 * Parameter:
 *   tid - Thread ID
 *
 * Return: Pointer ke thread, atau NULL
 */
thread_t *thread_cari(tid_t tid)
{
    proses_t *proses;
    thread_t *thread;
    
    if (!thread_initialized || tid == TID_TIDAK_ADA) {
        return NULL;
    }
    
    /* Cari di semua proses */
    proses = proses_dapat_kernel();
    while (proses != NULL) {
        thread = proses->threads;
        while (thread != NULL) {
            if (thread->tid == tid) {
                return thread;
            }
            thread = thread->next;
        }
        proses = proses->next;
    }
    
    return NULL;
}

/*
 * thread_cari_di_proses
 * ---------------------
 * Cari thread dalam proses tertentu.
 *
 * Parameter:
 *   pid - Process ID
 *   tid - Thread ID
 *
 * Return: Pointer ke thread, atau NULL
 */
thread_t *thread_cari_di_proses(pid_t pid, tid_t tid)
{
    proses_t *proses;
    thread_t *thread;
    
    if (!thread_initialized || tid == TID_TIDAK_ADA) {
        return NULL;
    }
    
    proses = proses_cari(pid);
    if (proses == NULL) {
        return NULL;
    }
    
    thread = proses->threads;
    while (thread != NULL) {
        if (thread->tid == tid) {
            return thread;
        }
        thread = thread->next;
    }
    
    return NULL;
}

/*
 * thread_valid
 * ------------
 * Validasi pointer thread.
 *
 * Parameter:
 *   thread - Pointer ke thread
 *
 * Return: BENAR jika valid
 */
bool_t thread_valid(thread_t *thread)
{
    if (thread == NULL) {
        return SALAH;
    }
    
    if (thread->magic != THREAD_MAGIC) {
        return SALAH;
    }
    
    if (thread->tid == 0) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * thread_set_status
 * -----------------
 * Set status thread.
 *
 * Parameter:
 *   thread  - Pointer ke thread
 *   status  - Status baru
 */
void thread_set_status(thread_t *thread, tak_bertanda32_t status)
{
    if (thread == NULL) {
        return;
    }
    
    thread->status = status;
}

/*
 * thread_dapat_statistik
 * ----------------------
 * Dapatkan statistik thread.
 *
 * Parameter:
 *   created - Pointer untuk jumlah thread dibuat
 *   exited  - Pointer untuk jumlah thread exit
 *   active  - Pointer untuk jumlah thread aktif
 */
void thread_dapat_statistik(tak_bertanda64_t *created,
                            tak_bertanda64_t *exited,
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
void thread_print_info(tid_t tid)
{
    thread_t *thread;
    proses_t *proses;
    const char *status_str;
    
    thread = thread_cari(tid);
    if (thread == NULL) {
        kernel_printf("[THREAD] Thread %lu tidak ditemukan\n",
                      (tak_bertanda64_t)tid);
        return;
    }
    
    proses = thread->proses;
    
    /* Status string */
    switch (thread->status) {
        case THREAD_STATUS_INVALID:
            status_str = "Invalid";
            break;
        case THREAD_STATUS_BELUM:
            status_str = "Belum";
            break;
        case THREAD_STATUS_SIAP:
            status_str = "Siap";
            break;
        case THREAD_STATUS_JALAN:
            status_str = "Jalan";
            break;
        case THREAD_STATUS_TUNGGU:
            status_str = "Tunggu";
            break;
        case THREAD_STATUS_STOP:
            status_str = "Stop";
            break;
        case THREAD_STATUS_BLOCK:
            status_str = "Block";
            break;
        case THREAD_STATUS_SLEEP:
            status_str = "Sleep";
            break;
        case THREAD_STATUS_ZOMBIE:
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
    kernel_printf("  Process:    %s\n",
                  proses != NULL ? proses->nama : "Unknown");
    kernel_printf("  Status:     %s\n", status_str);
    kernel_printf("  Prioritas:  %lu\n",
                  (tak_bertanda64_t)thread->prioritas);
    kernel_printf("  Quantum:    %lu\n",
                  (tak_bertanda64_t)thread->quantum);
    
    if (thread->stack_base != NULL) {
        kernel_printf("  Stack:      0x%p - 0x%p\n",
                      thread->stack_base,
                      (tak_bertanda8_t *)thread->stack_base +
                      thread->stack_size);
        kernel_printf("  Stack Size: %lu bytes\n",
                      (tak_bertanda64_t)thread->stack_size);
    }
    
    kernel_printf("  CPU Time:   %lu ticks\n", thread->cpu_time);
    kernel_printf("  Context Sw: %lu\n", thread->ctx_switches);
    
    if (thread->flags & THREAD_FLAG_KERNEL) {
        kernel_printf("  Type:       Kernel Thread\n");
    } else if (thread->flags & THREAD_FLAG_USER) {
        kernel_printf("  Type:       User Thread\n");
    }
    
    if (thread->flags & THREAD_FLAG_MAIN) {
        kernel_printf("  Role:       Main Thread\n");
    }
    
    if (thread->flags & THREAD_FLAG_DETACHED) {
        kernel_printf("  Detached:   Yes\n");
    }
    
    kernel_printf("========================================\n");
}

/*
 * thread_print_list
 * -----------------
 * Print list semua thread.
 */
void thread_print_list(void)
{
    proses_t *proses;
    thread_t *thread;
    const char *status_str;
    tak_bertanda32_t total = 0;
    
    kernel_printf("\n[THREAD] Daftar Thread:\n");
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_printf("  TID   PID   STATUS    PRIO  PROCESS\n");
    kernel_printf("----------------------------------------");
    kernel_printf("----------------------------------------\n");
    
    proses = proses_dapat_kernel();
    
    while (proses != NULL) {
        thread = proses->threads;
        
        while (thread != NULL) {
            switch (thread->status) {
                case THREAD_STATUS_BELUM:
                    status_str = "Belum";
                    break;
                case THREAD_STATUS_SIAP:
                    status_str = "Siap";
                    break;
                case THREAD_STATUS_JALAN:
                    status_str = "Jalan";
                    break;
                case THREAD_STATUS_TUNGGU:
                    status_str = "Tunggu";
                    break;
                case THREAD_STATUS_STOP:
                    status_str = "Stop";
                    break;
                case THREAD_STATUS_BLOCK:
                    status_str = "Block";
                    break;
                case THREAD_STATUS_SLEEP:
                    status_str = "Sleep";
                    break;
                case THREAD_STATUS_ZOMBIE:
                    status_str = "Zombie";
                    break;
                default:
                    status_str = "Unk";
                    break;
            }
            
            kernel_printf("  %5lu %5lu %-8s  %c    %s\n",
                          (tak_bertanda64_t)thread->tid,
                          (tak_bertanda64_t)thread->pid,
                          status_str,
                          thread->prioritas == PRIORITAS_REALTIME ? 'R' :
                          thread->prioritas == PRIORITAS_TINGGI ? 'T' :
                          thread->prioritas == PRIORITAS_NORMAL ? 'N' : 'L',
                          proses->nama);
            
            total++;
            thread = thread->next;
        }
        
        proses = proses->next;
    }
    
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_printf("Total: %lu threads\n", (tak_bertanda64_t)total);
    kernel_printf("\n");
}

/*
 * thread_alokasi_stack
 * --------------------
 * Alokasikan stack untuk thread (public interface).
 *
 * Parameter:
 *   size    - Ukuran stack
 *   is_user - Apakah stack untuk user mode
 *
 * Return: Pointer ke top of stack, atau NULL
 */
void *thread_alokasi_stack(ukuran_t size, bool_t is_user)
{
    return thread_alloc_stack_internal(size, is_user);
}

/*
 * thread_bebaskan_stack
 * ---------------------
 * Bebaskan stack thread (public interface).
 *
 * Parameter:
 *   stack   - Pointer ke stack
 *   size    - Ukuran stack
 *   is_user - Apakah stack user mode
 */
void thread_bebaskan_stack(void *stack, ukuran_t size, bool_t is_user)
{
    thread_free_stack_internal(stack, size, is_user);
}

/*
 * thread_alokasi_kstack
 * ---------------------
 * Alokasikan kernel stack.
 *
 * Return: Pointer ke stack, atau NULL
 */
void *thread_alokasi_kstack(void)
{
    void *kstack;
    
    kstack = kmalloc(STACK_UKURAN_KERNEL);
    if (kstack == NULL) {
        return NULL;
    }
    
    kernel_memset(kstack, 0, STACK_UKURAN_KERNEL);
    
    return (void *)((tak_bertanda8_t *)kstack + STACK_UKURAN_KERNEL);
}

/*
 * thread_bebaskan_kstack
 * ----------------------
 * Bebaskan kernel stack.
 *
 * Parameter:
 *   stack - Pointer ke stack
 */
void thread_bebaskan_kstack(void *stack)
{
    void *base;
    
    if (stack == NULL) {
        return;
    }
    
    base = (void *)((tak_bertanda8_t *)stack - STACK_UKURAN_KERNEL);
    kfree(base);
}
