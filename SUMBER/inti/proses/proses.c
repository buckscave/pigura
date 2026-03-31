/*
 * PIGURA OS - PROSES.C
 * ---------------------
 * Implementasi manajemen proses kernel.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mengelola
 * proses, termasuk pembuatan, penghancuran, dan pencarian proses
 * dengan dukungan penuh 32-bit dan 64-bit.
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

/* Ukuran nama proses maksimum */
#define NAMA_PROSES_MAKS        32

/* Ukuran path maksimum */
#define PATH_MAKS               256

/* Jumlah maksimum thread dalam pool */
#define THREAD_POOL_SIZE        (PROSES_MAKS * THREAD_MAKS_PER_PROSES)

/* PID range */
#define PID_MIN                 1
#define PID_MAX                 32768

/* Default resource limits */
#define DEFAULT_STACK_LIMIT     (8UL * 1024UL * 1024UL)
#define DEFAULT_DATA_LIMIT      (256UL * 1024UL * 1024UL)
#define DEFAULT_CORE_LIMIT      0
#define DEFAULT_MEMORY_LIMIT    (~(ukuran_t)0)
#define DEFAULT_CPU_LIMIT       0
#define DEFAULT_NOFILE_LIMIT    1024
#define DEFAULT_NPROC_LIMIT     4096

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Pool proses statis */
static proses_t g_pool_proses[PROSES_MAKS];

/* Pool thread statis */
static thread_t g_pool_thread[THREAD_POOL_SIZE];

/* List proses aktif */
static proses_t *g_proses_list = NULL;

/* List proses free */
static proses_t *g_proses_free = NULL;

/* List zombie */
static proses_t *g_zombie_list = NULL;

/* List thread free */
static thread_t *g_thread_free = NULL;

/* Proses khusus */
static proses_t *g_proses_kernel = NULL;
static proses_t *g_proses_idle = NULL;

/* Proses saat ini per CPU */
static proses_t *g_proses_current[CPU_MAKS];

/* Thread saat ini per CPU */
static thread_t *g_thread_current[CPU_MAKS];

/* Jumlah proses */
static tak_bertanda32_t g_proses_count = 0;

/* PID berikutnya */
static tak_bertanda32_t g_proses_next_pid = PID_MIN;

/* Thread ID berikutnya */
static tak_bertanda32_t g_thread_next_tid = 1;

/* Status inisialisasi */
static bool_t g_proses_diinit = SALAH;

/* Lock global */
static spinlock_t g_proses_lock;
static spinlock_t g_pid_lock;
static spinlock_t g_thread_lock;

/* Statistik */
static struct {
    tak_bertanda64_t total_dibuat;
    tak_bertanda64_t total_keluar;
    tak_bertanda64_t context_switches;
    tak_bertanda64_t fork_count;
    tak_bertanda64_t exec_count;
} g_statistik = {0, 0, 0, 0, 0};

/* Bitmap PID yang digunakan */
static tak_bertanda32_t g_pid_bitmap[(PID_MAX + 31) / 32];

/*
 * ============================================================================
 * FUNGSI INTERNAL PID (INTERNAL PID FUNCTIONS)
 * ============================================================================
 */

/*
 * pid_bitar_set
 * -------------
 * Set bit pada bitmap PID.
 *
 * Parameter:
 *   pid - PID yang akan di-set
 */
static void pid_bitar_set(tak_bertanda32_t pid)
{
    tak_bertanda32_t idx;
    tak_bertanda32_t bit;
    
    if (pid < PID_MIN || pid >= PID_MAX) {
        return;
    }
    
    idx = pid / 32;
    bit = pid % 32;
    
    g_pid_bitmap[idx] |= (1UL << bit);
}

/*
 * pid_bitar_clear
 * ---------------
 * Clear bit pada bitmap PID.
 *
 * Parameter:
 *   pid - PID yang akan di-clear
 */
static void pid_bitar_clear(tak_bertanda32_t pid)
{
    tak_bertanda32_t idx;
    tak_bertanda32_t bit;
    
    if (pid < PID_MIN || pid >= PID_MAX) {
        return;
    }
    
    idx = pid / 32;
    bit = pid % 32;
    
    g_pid_bitmap[idx] &= ~(1UL << bit);
}

/*
 * pid_bitar_test
 * --------------
 * Cek apakah PID sudah digunakan.
 *
 * Parameter:
 *   pid - PID yang akan dicek
 *
 * Return: BENAR jika sudah digunakan
 */
static bool_t pid_bitar_test(tak_bertanda32_t pid)
{
    tak_bertanda32_t idx;
    tak_bertanda32_t bit;
    
    if (pid < PID_MIN || pid >= PID_MAX) {
        return BENAR;
    }
    
    idx = pid / 32;
    bit = pid % 32;
    
    return (g_pid_bitmap[idx] & (1UL << bit)) ? BENAR : SALAH;
}

/*
 * pid_alokasi
 * -----------
 * Alokasikan PID baru.
 *
 * Return: PID baru, atau 0 jika gagal
 */
static tak_bertanda32_t pid_alokasi(void)
{
    tak_bertanda32_t pid;
    tak_bertanda32_t start;
    
    spinlock_kunci(&g_pid_lock);
    
    start = g_proses_next_pid;
    
    do {
        pid = g_proses_next_pid;
        g_proses_next_pid++;
        
        if (g_proses_next_pid >= PID_MAX) {
            g_proses_next_pid = PID_MIN;
        }
        
        if (!pid_bitar_test(pid)) {
            pid_bitar_set(pid);
            spinlock_buka(&g_pid_lock);
            return pid;
        }
    } while (g_proses_next_pid != start);
    
    spinlock_buka(&g_pid_lock);
    
    return 0;
}

/*
 * pid_bebaskan
 * ------------
 * Bebaskan PID.
 *
 * Parameter:
 *   pid - PID yang akan dibebaskan
 */
static void pid_bebaskan(tak_bertanda32_t pid)
{
    if (pid >= PID_MIN && pid < PID_MAX) {
        spinlock_kunci(&g_pid_lock);
        pid_bitar_clear(pid);
        spinlock_buka(&g_pid_lock);
    }
}

/*
 * ============================================================================
 * FUNGSI INTERNAL THREAD (INTERNAL THREAD FUNCTIONS)
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
    
    total = THREAD_POOL_SIZE;
    g_thread_free = NULL;
    
    for (i = 0; i < total; i++) {
        g_pool_thread[i].magic = THREAD_MAGIC;
        g_pool_thread[i].tid = 0;
        g_pool_thread[i].pid = 0;
        g_pool_thread[i].status = THREAD_STATUS_INVALID;
        g_pool_thread[i].flags = THREAD_FLAG_NONE;
        g_pool_thread[i].next = g_thread_free;
        g_pool_thread[i].prev = NULL;
        g_thread_free = &g_pool_thread[i];
    }
}

/*
 * thread_alokasi_internal
 * -----------------------
 * Alokasikan deskriptor thread dari pool.
 *
 * Return: Pointer ke thread, atau NULL
 */
thread_t *thread_alokasi(void)
{
    thread_t *thread;
    
    spinlock_kunci(&g_thread_lock);
    
    if (g_thread_free == NULL) {
        spinlock_buka(&g_thread_lock);
        return NULL;
    }
    
    thread = g_thread_free;
    g_thread_free = thread->next;
    
    if (g_thread_free != NULL) {
        g_thread_free->prev = NULL;
    }
    
    /* Reset semua field */
    thread->magic = THREAD_MAGIC;
    thread->tid = g_thread_next_tid++;
    thread->pid = 0;
    thread->status = THREAD_STATUS_BELUM;
    thread->flags = THREAD_FLAG_NONE;
    thread->prioritas = PRIORITAS_NORMAL;
    thread->nice = NICE_DEFAULT;
    thread->quantum = QUANTUM_NORMAL;
    thread->context = NULL;
    thread->stack_base = NULL;
    thread->stack_ptr = NULL;
    thread->stack_size = 0;
    thread->kstack_base = NULL;
    thread->kstack_ptr = NULL;
    thread->kstack_size = 0;
    thread->cpu_time = 0;
    thread->start_time = 0;
    thread->wake_time = 0;
    thread->block_reason = BLOCK_ALASAN_NONE;
    thread->exit_code = 0;
    thread->next = NULL;
    thread->prev = NULL;
    thread->waiting = NULL;
    thread->retval = NULL;
    thread->tls = NULL;
    thread->tls_size = 0;
    thread->signal_mask = 0;
    thread->signal_pending = 0;
    thread->proses = NULL;
    thread->cpu_affinity = 0xFFFFFFFF;
    thread->last_cpu = 0;
    thread->ctx_switches = 0;
    
    spinlock_buka(&g_thread_lock);
    
    return thread;
}

/*
 * thread_bebaskan
 * ---------------
 * Bebaskan deskriptor thread ke pool.
 *
 * Parameter:
 *   thread - Pointer ke thread
 */
void thread_bebaskan(thread_t *thread)
{
    if (thread == NULL) {
        return;
    }
    
    spinlock_kunci(&g_thread_lock);
    
    thread->magic = THREAD_MAGIC;
    thread->tid = 0;
    thread->status = THREAD_STATUS_INVALID;
    thread->flags = THREAD_FLAG_NONE;
    thread->next = g_thread_free;
    thread->prev = NULL;
    
    if (g_thread_free != NULL) {
        g_thread_free->prev = thread;
    }
    
    g_thread_free = thread;
    
    spinlock_buka(&g_thread_lock);
}

/*
 * ============================================================================
 * FUNGSI INTERNAL PROSES (INTERNAL PROCESS FUNCTIONS)
 * ============================================================================
 */

/*
 * proses_pool_init
 * ----------------
 * Inisialisasi pool proses.
 */
static void proses_pool_init(void)
{
    tak_bertanda32_t i;
    
    g_proses_list = NULL;
    g_proses_free = NULL;
    g_zombie_list = NULL;
    g_proses_count = 0;
    g_proses_next_pid = PID_MIN;
    
    kernel_memset(g_pid_bitmap, 0, sizeof(g_pid_bitmap));
    
    for (i = 0; i < PROSES_MAKS; i++) {
        g_pool_proses[i].magic = PROSES_MAGIC;
        g_pool_proses[i].pid = 0;
        g_pool_proses[i].ppid = 0;
        g_pool_proses[i].next = g_proses_free;
        g_pool_proses[i].prev = NULL;
        g_proses_free = &g_pool_proses[i];
    }
}

/*
 * proses_alokasi_internal
 * -----------------------
 * Alokasikan deskriptor proses dari pool.
 *
 * Return: Pointer ke proses, atau NULL
 */
static proses_t *proses_alokasi_internal(void)
{
    proses_t *proses;
    tak_bertanda32_t i;
    pid_t pid;
    
    spinlock_kunci(&g_proses_lock);
    
    if (g_proses_free == NULL) {
        spinlock_buka(&g_proses_lock);
        return NULL;
    }
    
    /* Alokasikan PID */
    pid = pid_alokasi();
    if (pid == 0) {
        spinlock_buka(&g_proses_lock);
        return NULL;
    }
    
    proses = g_proses_free;
    g_proses_free = proses->next;
    
    if (g_proses_free != NULL) {
        g_proses_free->prev = NULL;
    }
    
    /* Reset semua field */
    proses->magic = PROSES_MAGIC;
    proses->pid = pid;
    proses->ppid = 0;
    proses->uid = 0;
    proses->gid = 0;
    proses->euid = 0;
    proses->egid = 0;
    proses->suid = 0;
    proses->sgid = 0;
    proses->fsuid = 0;
    proses->fsgid = 0;
    proses->pgid = 0;
    proses->sid = 0;
    
    proses->nama[0] = '\0';
    proses->cwd[0] = '\0';
    proses->root[0] = '\0';
    proses->exe[0] = '\0';
    
    proses->flags = PROSES_FLAG_NONE;
    proses->status = PROSES_STATUS_BELUM;
    proses->prioritas = PRIORITAS_NORMAL;
    proses->nice = NICE_DEFAULT;
    proses->exit_code = 0;
    proses->exit_signal = 0;
    
    proses->vm = NULL;
    proses->pgdir = NULL;
    proses->entry_point = 0;
    proses->brk_start = 0;
    proses->brk_current = 0;
    proses->brk_end = 0;
    proses->start_code = 0;
    proses->end_code = 0;
    proses->start_data = 0;
    proses->end_data = 0;
    proses->start_stack = 0;
    
    proses->threads = NULL;
    proses->main_thread = NULL;
    proses->thread_count = 0;
    proses->thread_limit = THREAD_MAKS_PER_PROSES;
    
    /* Reset FD table */
    proses->fd_count = 0;
    proses->fd_next_free = 0;
    proses->fd_max = FD_MAKS_PER_PROSES;
    for (i = 0; i < FD_MAKS_PER_PROSES; i++) {
        proses->fd_table[i] = NULL;
    }
    
    /* Reset signal */
    proses->signal_mask = 0;
    proses->signal_pending = 0;
    proses->signal_ignored = 0;
    proses->signal_trampoline = NULL;
    proses->saved_signal_mask = 0;
    proses->signal_count = 0;
    for (i = 0; i < SIGNAL_JUMLAH; i++) {
        proses->signal_handlers[i].handler = SIGNAL_HANDLER_DEFAULT;
        proses->signal_handlers[i].flags = 0;
        proses->signal_handlers[i].mask = 0;
        proses->signal_handlers[i].restorer = NULL;
        proses->signal_handlers[i].call_count = 0;
    }
    
    /* Reset scheduling */
    proses->cpu_time = 0;
    proses->start_time = 0;
    proses->end_time = 0;
    proses->real_start = 0;
    proses->quantum = QUANTUM_NORMAL;
    proses->rt_priority = 0;
    proses->cpu_affinity = 0xFFFFFFFF;
    proses->last_cpu = 0;
    
    /* Reset list */
    proses->next = NULL;
    proses->prev = NULL;
    proses->children = NULL;
    proses->sibling = NULL;
    proses->parent = NULL;
    proses->child_count = 0;
    
    /* Reset wait state */
    proses->wait_state = 0;
    proses->wait_pid = 0;
    proses->wait_options = 0;
    proses->zombie_children = 0;
    
    /* Reset vfork */
    proses->vfork_child = 0;
    
    /* Reset capabilities */
    proses->cap_effective = 0;
    proses->cap_permitted = 0;
    proses->cap_inheritable = 0;
    
    /* Set resource limits */
    proses->limit_stack = DEFAULT_STACK_LIMIT;
    proses->limit_data = DEFAULT_DATA_LIMIT;
    proses->limit_core = DEFAULT_CORE_LIMIT;
    proses->limit_memory = DEFAULT_MEMORY_LIMIT;
    proses->limit_cpu = DEFAULT_CPU_LIMIT;
    proses->limit_nofile = DEFAULT_NOFILE_LIMIT;
    proses->limit_nproc = DEFAULT_NPROC_LIMIT;
    
    /* Reset statistik */
    kernel_memset(&proses->mem_stat, 0, sizeof(statistik_memori_t));
    kernel_memset(&proses->cpu_stat, 0, sizeof(statistik_cpu_t));
    kernel_memset(&proses->io_stat, 0, sizeof(statistik_io_t));
    
    /* Reset security */
    proses->securebits = 0;
    proses->seccomp_filter = NULL;
    
    /* Reset namespace */
    proses->ns_pid = NULL;
    proses->ns_mnt = NULL;
    proses->ns_net = NULL;
    
    /* Reset groups */
    proses->ngroups = 0;
    for (i = 0; i < 32; i++) {
        proses->groups[i] = 0;
    }
    
    spinlock_buka(&g_proses_lock);
    
    return proses;
}

/*
 * proses_bebaskan_internal
 * ------------------------
 * Bebaskan deskriptor proses ke pool.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
static void proses_bebaskan_internal(proses_t *proses)
{
    if (proses == NULL) {
        return;
    }
    
    /* Bebaskan PID */
    pid_bebaskan(proses->pid);
    
    spinlock_kunci(&g_proses_lock);
    
    proses->magic = PROSES_MAGIC;
    proses->pid = 0;
    proses->status = PROSES_STATUS_INVALID;
    proses->next = g_proses_free;
    proses->prev = NULL;
    
    if (g_proses_free != NULL) {
        g_proses_free->prev = proses;
    }
    
    g_proses_free = proses;
    
    spinlock_buka(&g_proses_lock);
}

/*
 * ============================================================================
 * FUNGSI PUBLIK PROSES (PUBLIC PROCESS FUNCTIONS)
 * ============================================================================
 */

/*
 * proses_init
 * -----------
 * Inisialisasi subsistem proses.
 *
 * Return: Status operasi
 */
status_t proses_init(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t cpu_count;
    
    if (g_proses_diinit) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Inisialisasi pool */
    proses_pool_init();
    thread_pool_init();
    
    /* Init lock */
    spinlock_init(&g_proses_lock);
    spinlock_init(&g_pid_lock);
    spinlock_init(&g_thread_lock);
    
    /* Reset statistik */
    kernel_memset(&g_statistik, 0, sizeof(g_statistik));
    
    /* Reset current pointers */
    cpu_count = hal_cpu_get_count();
    for (i = 0; i < cpu_count && i < CPU_MAKS; i++) {
        g_proses_current[i] = NULL;
        g_thread_current[i] = NULL;
    }
    
    /* Buat proses kernel */
    g_proses_kernel = proses_alokasi_internal();
    if (g_proses_kernel == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    g_proses_kernel->pid = PID_KERNEL;
    g_proses_kernel->ppid = 0;
    g_proses_kernel->gid = 0;
    g_proses_kernel->uid = 0;
    g_proses_kernel->euid = 0;
    g_proses_kernel->egid = 0;
    kernel_strncpy(g_proses_kernel->nama, "kernel", NAMA_PROSES_MAKS - 1);
    g_proses_kernel->nama[NAMA_PROSES_MAKS - 1] = '\0';
    g_proses_kernel->flags = PROSES_FLAG_KERNEL | PROSES_FLAG_PRIVILEGED;
    g_proses_kernel->status = PROSES_STATUS_JALAN;
    g_proses_kernel->prioritas = PRIORITAS_REALTIME;
    g_proses_kernel->quantum = QUANTUM_REALTIME;
    kernel_strncpy(g_proses_kernel->cwd, "/", PATH_MAKS - 1);
    kernel_strncpy(g_proses_kernel->root, "/", PATH_MAKS - 1);
    
    /* Proses kernel tidak memiliki VM terpisah */
    g_proses_kernel->vm = NULL;
    
    /* Set waktu mulai */
    g_proses_kernel->start_time = hal_timer_get_ticks();
    g_proses_kernel->real_start = hal_timer_get_ticks();
    
    /* Tambah ke list */
    proses_tambah_ke_list(g_proses_kernel);
    
    /* Set PID 0 sebagai reserved */
    pid_bitar_set(PID_KERNEL);
    
    /* Buat proses idle */
    g_proses_idle = proses_alokasi_internal();
    if (g_proses_idle == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    g_proses_idle->pid = PID_IDLE;
    g_proses_idle->ppid = PID_KERNEL;
    kernel_strncpy(g_proses_idle->nama, "idle", NAMA_PROSES_MAKS - 1);
    g_proses_idle->nama[NAMA_PROSES_MAKS - 1] = '\0';
    g_proses_idle->flags = PROSES_FLAG_KERNEL;
    g_proses_idle->status = PROSES_STATUS_SIAP;
    g_proses_idle->prioritas = PRIORITAS_IDLE;
    g_proses_idle->quantum = QUANTUM_IDLE;
    g_proses_idle->start_time = hal_timer_get_ticks();
    g_proses_idle->real_start = hal_timer_get_ticks();
    kernel_strncpy(g_proses_idle->cwd, "/", PATH_MAKS - 1);
    kernel_strncpy(g_proses_idle->root, "/", PATH_MAKS - 1);
    
    proses_tambah_ke_list(g_proses_idle);
    proses_tambah_child(g_proses_kernel, g_proses_idle);
    
    /* Set PID 2 sebagai reserved */
    pid_bitar_set(PID_IDLE);
    
    /* Set proses saat ini */
    g_proses_current[0] = g_proses_kernel;
    
    g_proses_diinit = BENAR;
    
    kernel_printf("[PROSES] Process manager initialized\n");
    kernel_printf("         Kernel PID: %lu, Idle PID: %lu\n",
                  (tak_bertanda64_t)g_proses_kernel->pid,
                  (tak_bertanda64_t)g_proses_idle->pid);
    kernel_printf("         Max processes: %lu, Max threads: %lu\n",
                  (tak_bertanda64_t)PROSES_MAKS,
                  (tak_bertanda64_t)THREAD_MAKS_PER_PROSES);
    
    return STATUS_BERHASIL;
}

/*
 * proses_diinit
 * -------------
 * Cek apakah subsistem proses sudah diinisialisasi.
 *
 * Return: Status operasi
 */
status_t proses_diinit(void)
{
    return g_proses_diinit ? STATUS_BERHASIL : STATUS_GAGAL;
}

/*
 * proses_buat
 * -----------
 * Buat proses baru.
 *
 * Parameter:
 *   nama  - Nama proses
 *   ppid  - Parent process ID
 *   flags - Flag proses
 *
 * Return: PID proses baru, atau PID_TIDAK_ADA jika gagal
 */
pid_t proses_buat(const char *nama, pid_t ppid, tak_bertanda32_t flags)
{
    proses_t *proses;
    proses_t *parent;
    thread_t *main_thread;
    tak_bertanda32_t i;
    
    if (!g_proses_diinit) {
        return PID_TIDAK_ADA;
    }
    
    spinlock_kunci(&g_proses_lock);
    
    /* Alokasikan deskriptor proses */
    proses = proses_alokasi_internal();
    if (proses == NULL) {
        spinlock_buka(&g_proses_lock);
        return PID_TIDAK_ADA;
    }
    
    /* Set nama */
    if (nama != NULL && nama[0] != '\0') {
        kernel_strncpy(proses->nama, nama, NAMA_PROSES_MAKS - 1);
        proses->nama[NAMA_PROSES_MAKS - 1] = '\0';
    } else {
        kernel_strncpy(proses->nama, "unknown", NAMA_PROSES_MAKS - 1);
    }
    
    /* Cari parent */
    parent = proses_cari(ppid);
    if (parent != NULL) {
        proses->ppid = ppid;
        proses->gid = parent->gid;
        proses->uid = parent->uid;
        proses->euid = parent->euid;
        proses->egid = parent->egid;
        proses->suid = parent->suid;
        proses->sgid = parent->sgid;
        proses->fsuid = parent->fsuid;
        proses->fsgid = parent->fsgid;
        proses->pgid = parent->pgid;
        proses->sid = parent->sid;
        kernel_strncpy(proses->cwd, parent->cwd, PATH_MAKS - 1);
        kernel_strncpy(proses->root, parent->root, PATH_MAKS - 1);
        
        /* Copy groups */
        proses->ngroups = parent->ngroups;
        for (i = 0; i < parent->ngroups && i < 32; i++) {
            proses->groups[i] = parent->groups[i];
        }
        
        /* Copy capabilities */
        proses->cap_effective = parent->cap_effective;
        proses->cap_permitted = parent->cap_permitted;
        proses->cap_inheritable = parent->cap_inheritable;
    } else {
        proses->ppid = PID_KERNEL;
        kernel_strncpy(proses->cwd, "/", PATH_MAKS - 1);
        kernel_strncpy(proses->root, "/", PATH_MAKS - 1);
    }
    
    proses->flags = flags;
    proses->status = PROSES_STATUS_BELUM;
    proses->prioritas = PRIORITAS_NORMAL;
    proses->quantum = QUANTUM_NORMAL;
    proses->start_time = hal_timer_get_ticks();
    proses->real_start = hal_timer_get_ticks();
    
    /* Alokasikan main thread */
    main_thread = thread_alokasi();
    if (main_thread == NULL) {
        proses_bebaskan_internal(proses);
        spinlock_buka(&g_proses_lock);
        return PID_TIDAK_ADA;
    }
    
    main_thread->pid = proses->pid;
    main_thread->tid = proses->pid;  /* TID = PID untuk main thread */
    main_thread->proses = proses;
    main_thread->flags = THREAD_FLAG_MAIN;
    main_thread->status = THREAD_STATUS_SIAP;
    main_thread->prioritas = proses->prioritas;
    main_thread->quantum = proses->quantum;
    main_thread->stack_size = STACK_UKURAN_USER;
    
    proses->main_thread = main_thread;
    proses->threads = main_thread;
    proses->thread_count = 1;
    
    /* Tambah ke list */
    proses_tambah_ke_list(proses);
    
    /* Tambah sebagai child */
    if (parent != NULL) {
        proses_tambah_child(parent, proses);
    }
    
    g_statistik.total_dibuat++;
    
    spinlock_buka(&g_proses_lock);
    
    return proses->pid;
}

/*
 * proses_hapus
 * ------------
 * Hapus proses dari sistem.
 *
 * Parameter:
 *   pid - Process ID
 *
 * Return: Status operasi
 */
status_t proses_hapus(pid_t pid)
{
    proses_t *proses;
    thread_t *thread;
    thread_t *next_thread;
    tak_bertanda32_t i;
    
    if (!g_proses_diinit) {
        return STATUS_GAGAL;
    }
    
    spinlock_kunci(&g_proses_lock);
    
    proses = proses_cari(pid);
    if (proses == NULL) {
        spinlock_buka(&g_proses_lock);
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Tidak boleh hapus proses kernel */
    if (proses->flags & PROSES_FLAG_KERNEL) {
        spinlock_buka(&g_proses_lock);
        return STATUS_AKSES_DITOLAK;
    }
    
    /* Bebaskan threads */
    thread = proses->threads;
    while (thread != NULL) {
        next_thread = thread->next;
        
        if (thread->context != NULL) {
            context_hancurkan(thread->context);
        }
        
        if (thread->stack_base != NULL) {
            thread_bebaskan_stack(thread->stack_base,
                                   thread->stack_size,
                                   !(proses->flags & PROSES_FLAG_KERNEL));
        }
        
        if (thread->kstack_base != NULL) {
            thread_bebaskan_kstack(thread->kstack_base);
        }
        
        thread_bebaskan(thread);
        thread = next_thread;
    }
    
    /* Tutup file descriptors */
    for (i = 0; i < FD_MAKS_PER_PROSES; i++) {
        if (proses->fd_table[i] != NULL) {
            proses->fd_table[i] = NULL;
        }
    }
    
    /* Hapus dari list */
    proses_hapus_dari_list(proses);
    
    /* Hapus dari parent */
    if (proses->parent != NULL) {
        proses_hapus_child(proses->parent, proses);
    }
    
    /* Bebaskan deskriptor */
    proses_bebaskan_internal(proses);
    
    g_statistik.total_keluar++;
    
    spinlock_buka(&g_proses_lock);
    
    return STATUS_BERHASIL;
}

/*
 * proses_bebaskan
 * ---------------
 * Bebaskan struktur proses (public interface).
 * Fungsi ini dipanggil untuk membersihkan proses zombie
 * setelah proses parent melakukan wait().
 *
 * Parameter:
 *   proses - Pointer ke proses yang akan dibebaskan
 */
void proses_bebaskan(proses_t *proses)
{
    if (proses == NULL) {
        return;
    }
    
    /* Validasi magic number */
    if (proses->magic != PROSES_MAGIC) {
        return;
    }
    
    /* Hanya bebaskan proses zombie */
    if (proses->status != PROSES_STATUS_ZOMBIE) {
        return;
    }
    
    /* Bebaskan semua resource */
    proses_bebaskan_internal(proses);
}

/*
 * proses_cari
 * -----------
 * Cari proses berdasarkan PID.
 *
 * Parameter:
 *   pid - Process ID
 *
 * Return: Pointer ke proses, atau NULL
 */
proses_t *proses_cari(pid_t pid)
{
    proses_t *proses;
    
    if (!g_proses_diinit || pid == 0) {
        return NULL;
    }
    
    /* Special case untuk kernel process */
    if (pid == PID_KERNEL) {
        return g_proses_kernel;
    }
    
    /* Special case untuk idle process */
    if (pid == PID_IDLE) {
        return g_proses_idle;
    }
    
    /* Cari di list aktif */
    proses = g_proses_list;
    while (proses != NULL) {
        if (proses->pid == pid) {
            return proses;
        }
        proses = proses->next;
    }
    
    /* Cari di zombie list */
    proses = g_zombie_list;
    while (proses != NULL) {
        if (proses->pid == pid) {
            return proses;
        }
        proses = proses->next;
    }
    
    return NULL;
}

/*
 * proses_cari_by_name
 * -------------------
 * Cari proses berdasarkan nama.
 *
 * Parameter:
 *   nama - Nama proses
 *
 * Return: Pointer ke proses, atau NULL
 */
proses_t *proses_cari_by_name(const char *nama)
{
    proses_t *proses;
    
    if (!g_proses_diinit || nama == NULL) {
        return NULL;
    }
    
    proses = g_proses_list;
    while (proses != NULL) {
        if (kernel_strcmp(proses->nama, nama) == 0) {
            return proses;
        }
        proses = proses->next;
    }
    
    return NULL;
}

/*
 * proses_dapat_saat_ini
 * ---------------------
 * Dapatkan proses saat ini.
 *
 * Return: Pointer ke proses saat ini
 */
proses_t *proses_dapat_saat_ini(void)
{
    tak_bertanda32_t cpu_id;
    
    cpu_id = hal_cpu_get_id();
    
    if (cpu_id >= CPU_MAKS) {
        cpu_id = 0;
    }
    
    return g_proses_current[cpu_id];
}

/*
 * proses_dapat_kernel
 * -------------------
 * Dapatkan proses kernel.
 *
 * Return: Pointer ke proses kernel
 */
proses_t *proses_dapat_kernel(void)
{
    return g_proses_kernel;
}

/*
 * proses_dapat_idle
 * -----------------
 * Dapatkan proses idle.
 *
 * Return: Pointer ke proses idle
 */
proses_t *proses_dapat_idle(void)
{
    return g_proses_idle;
}

/*
 * proses_dapat_parent
 * -------------------
 * Dapatkan parent dari proses.
 *
 * Parameter:
 *   child - Pointer ke child proses
 *
 * Return: Pointer ke parent, atau NULL
 */
proses_t *proses_dapat_parent(proses_t *child)
{
    if (child == NULL) {
        return NULL;
    }
    
    return child->parent;
}

/*
 * proses_set_saat_ini
 * -------------------
 * Set proses saat ini.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
void proses_set_saat_ini(proses_t *proses)
{
    tak_bertanda32_t cpu_id;
    
    cpu_id = hal_cpu_get_id();
    
    if (cpu_id >= CPU_MAKS) {
        cpu_id = 0;
    }
    
    g_proses_current[cpu_id] = proses;
    
    if (proses != NULL && proses->main_thread != NULL) {
        g_thread_current[cpu_id] = proses->main_thread;
    }
}

/*
 * proses_dapat_jumlah
 * -------------------
 * Dapatkan jumlah proses aktif.
 *
 * Return: Jumlah proses
 */
tak_bertanda32_t proses_dapat_jumlah(void)
{
    return g_proses_count;
}

/*
 * proses_dapat_jumlah_zombie
 * --------------------------
 * Dapatkan jumlah proses zombie.
 *
 * Return: Jumlah zombie
 */
tak_bertanda32_t proses_dapat_jumlah_zombie(void)
{
    tak_bertanda32_t count = 0;
    proses_t *proses;
    
    proses = g_zombie_list;
    while (proses != NULL) {
        count++;
        proses = proses->next;
    }
    
    return count;
}

/*
 * proses_dapat_jumlah_total
 * -------------------------
 * Dapatkan total proses yang pernah dibuat.
 *
 * Return: Jumlah total
 */
tak_bertanda32_t proses_dapat_jumlah_total(void)
{
    return (tak_bertanda32_t)g_statistik.total_dibuat;
}

/*
 * proses_exit
 * -----------
 * Exit proses.
 *
 * Parameter:
 *   pid       - Process ID
 *   exit_code - Exit code
 *
 * Return: Status operasi
 */
status_t proses_exit(pid_t pid, tanda32_t exit_code)
{
    proses_t *proses;
    proses_t *parent;
    thread_t *thread;
    thread_t *next;
    tak_bertanda32_t i;
    
    if (!g_proses_diinit) {
        return STATUS_GAGAL;
    }
    
    spinlock_kunci(&g_proses_lock);
    
    proses = proses_cari(pid);
    if (proses == NULL) {
        spinlock_buka(&g_proses_lock);
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Tidak boleh exit kernel process */
    if (proses->flags & PROSES_FLAG_KERNEL) {
        spinlock_buka(&g_proses_lock);
        return STATUS_AKSES_DITOLAK;
    }
    
    /* Set status zombie */
    proses->status = PROSES_STATUS_ZOMBIE;
    proses->exit_code = exit_code;
    proses->flags |= PROSES_FLAG_ZOMBIE;
    proses->end_time = hal_timer_get_ticks();
    
    /* Update threads */
    thread = proses->threads;
    while (thread != NULL) {
        next = thread->next;
        thread->status = THREAD_STATUS_ZOMBIE;
        thread = next;
    }
    
    /* Tutup file descriptors */
    for (i = 0; i < FD_MAKS_PER_PROSES; i++) {
        if (proses->fd_table[i] != NULL) {
            proses->fd_table[i] = NULL;
        }
    }
    proses->fd_count = 0;
    
    /* Notify parent */
    parent = proses_cari(proses->ppid);
    if (parent != NULL) {
        parent->zombie_children++;
        
        /* Kirim SIGCHLD ke parent */
        signal_kirim_ke_proses(parent, SIGNAL_CHILD);
    }
    
    g_statistik.total_keluar++;
    
    /* Hapus dari list aktif */
    proses_hapus_dari_list(proses);
    
    /* Tambah ke zombie list */
    proses->next = g_zombie_list;
    proses->prev = NULL;
    if (g_zombie_list != NULL) {
        g_zombie_list->prev = proses;
    }
    g_zombie_list = proses;
    
    spinlock_buka(&g_proses_lock);
    
    return STATUS_BERHASIL;
}

/*
 * proses_kill
 * -----------
 * Kirim signal ke proses.
 *
 * Parameter:
 *   pid    - Process ID
 *   signal - Nomor signal
 *
 * Return: Status operasi
 */
status_t proses_kill(pid_t pid, tak_bertanda32_t signal)
{
    proses_t *proses;
    
    if (!g_proses_diinit) {
        return STATUS_GAGAL;
    }
    
    proses = proses_cari(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    /* Tidak boleh kill kernel process */
    if (proses->flags & PROSES_FLAG_KERNEL) {
        return STATUS_AKSES_DITOLAK;
    }
    
    /* Handle signal */
    switch (signal) {
        case SIGNAL_KILL:
        case SIGNAL_TERM:
            return proses_exit(pid, 128 + signal);
            
        case SIGNAL_STOP:
            proses->status = PROSES_STATUS_STOP;
            proses->flags |= PROSES_FLAG_STOPPED;
            return STATUS_BERHASIL;
            
        case SIGNAL_CONT:
            if (proses->status == PROSES_STATUS_STOP) {
                proses->status = PROSES_STATUS_SIAP;
                proses->flags &= ~PROSES_FLAG_STOPPED;
                proses->flags |= PROSES_FLAG_CONTINUED;
            }
            return STATUS_BERHASIL;
            
        default:
            return signal_kirim(pid, signal);
    }
}

/*
 * proses_set_prioritas
 * --------------------
 * Set prioritas proses.
 *
 * Parameter:
 *   pid       - Process ID
 *   prioritas - Prioritas baru
 *
 * Return: Status operasi
 */
status_t proses_set_prioritas(pid_t pid, tak_bertanda32_t prioritas)
{
    proses_t *proses;
    
    if (!g_proses_diinit) {
        return STATUS_GAGAL;
    }
    
    if (prioritas > PRIORITAS_KRITIS) {
        return STATUS_PARAM_INVALID;
    }
    
    spinlock_kunci(&g_proses_lock);
    
    proses = proses_cari(pid);
    if (proses == NULL) {
        spinlock_buka(&g_proses_lock);
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    proses->prioritas = prioritas;
    
    /* Update main thread */
    if (proses->main_thread != NULL) {
        proses->main_thread->prioritas = prioritas;
    }
    
    spinlock_buka(&g_proses_lock);
    
    return STATUS_BERHASIL;
}

/*
 * proses_dapat_prioritas
 * ----------------------
 * Dapatkan prioritas proses.
 *
 * Parameter:
 *   pid - Process ID
 *
 * Return: Prioritas proses
 */
tak_bertanda32_t proses_dapat_prioritas(pid_t pid)
{
    proses_t *proses;
    
    proses = proses_cari(pid);
    if (proses == NULL) {
        return PRIORITAS_NORMAL;
    }
    
    return proses->prioritas;
}

/*
 * proses_set_nice
 * ---------------
 * Set nice value proses.
 *
 * Parameter:
 *   pid  - Process ID
 *   nice - Nice value (-20 sampai 19)
 *
 * Return: Status operasi
 */
status_t proses_set_nice(pid_t pid, tanda32_t nice)
{
    proses_t *proses;
    
    if (!g_proses_diinit) {
        return STATUS_GAGAL;
    }
    
    if (nice < NICE_MIN || nice > NICE_MAX) {
        return STATUS_PARAM_INVALID;
    }
    
    spinlock_kunci(&g_proses_lock);
    
    proses = proses_cari(pid);
    if (proses == NULL) {
        spinlock_buka(&g_proses_lock);
        return STATUS_TIDAK_DITEMUKAN;
    }
    
    proses->nice = nice;
    
    spinlock_buka(&g_proses_lock);
    
    return STATUS_BERHASIL;
}

/*
 * proses_dapat_nice
 * -----------------
 * Dapatkan nice value proses.
 *
 * Parameter:
 *   pid - Process ID
 *
 * Return: Nice value
 */
tanda32_t proses_dapat_nice(pid_t pid)
{
    proses_t *proses;
    
    proses = proses_cari(pid);
    if (proses == NULL) {
        return NICE_DEFAULT;
    }
    
    return proses->nice;
}

/*
 * proses_tambah_child
 * -------------------
 * Tambahkan child proses.
 *
 * Parameter:
 *   parent - Pointer ke parent proses
 *   child  - Pointer ke child proses
 */
void proses_tambah_child(proses_t *parent, proses_t *child)
{
    if (parent == NULL || child == NULL) {
        return;
    }
    
    child->parent = parent;
    child->sibling = parent->children;
    parent->children = child;
    parent->child_count++;
}

/*
 * proses_hapus_child
 * ------------------
 * Hapus child proses.
 *
 * Parameter:
 *   parent - Pointer ke parent proses
 *   child  - Pointer ke child proses
 */
void proses_hapus_child(proses_t *parent, proses_t *child)
{
    proses_t *curr;
    proses_t *prev;
    
    if (parent == NULL || child == NULL) {
        return;
    }
    
    prev = NULL;
    curr = parent->children;
    
    while (curr != NULL && curr != child) {
        prev = curr;
        curr = curr->sibling;
    }
    
    if (curr == child) {
        if (prev == NULL) {
            parent->children = child->sibling;
        } else {
            prev->sibling = child->sibling;
        }
        child->sibling = NULL;
        child->parent = NULL;
        if (parent->child_count > 0) {
            parent->child_count--;
        }
    }
}

/*
 * proses_dapat_child
 * ------------------
 * Dapatkan child proses berdasarkan PID.
 *
 * Parameter:
 *   parent - Pointer ke parent proses
 *   pid    - PID child
 *
 * Return: Pointer ke child, atau NULL
 */
proses_t *proses_dapat_child(proses_t *parent, pid_t pid)
{
    proses_t *child;
    
    if (parent == NULL) {
        return NULL;
    }
    
    child = parent->children;
    while (child != NULL) {
        if (child->pid == pid) {
            return child;
        }
        child = child->sibling;
    }
    
    return NULL;
}

/*
 * proses_tambah_ke_list
 * ---------------------
 * Tambahkan proses ke list aktif.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
void proses_tambah_ke_list(proses_t *proses)
{
    if (proses == NULL) {
        return;
    }
    
    proses->next = g_proses_list;
    proses->prev = NULL;
    
    if (g_proses_list != NULL) {
        g_proses_list->prev = proses;
    }
    
    g_proses_list = proses;
    g_proses_count++;
}

/*
 * proses_hapus_dari_list
 * ----------------------
 * Hapus proses dari list aktif.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
void proses_hapus_dari_list(proses_t *proses)
{
    if (proses == NULL) {
        return;
    }
    
    if (proses->prev != NULL) {
        proses->prev->next = proses->next;
    } else {
        g_proses_list = proses->next;
    }
    
    if (proses->next != NULL) {
        proses->next->prev = proses->prev;
    }
    
    proses->next = NULL;
    proses->prev = NULL;
    
    if (g_proses_count > 0) {
        g_proses_count--;
    }
}

/*
 * proses_valid
 * ------------
 * Validasi pointer proses.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: BENAR jika valid
 */
bool_t proses_valid(proses_t *proses)
{
    if (proses == NULL) {
        return SALAH;
    }
    
    if (proses->magic != PROSES_MAGIC) {
        return SALAH;
    }
    
    if (proses->pid == 0 || proses->pid >= PID_MAX) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * proses_pid_valid
 * ----------------
 * Validasi PID.
 *
 * Parameter:
 *   pid - PID yang divalidasi
 *
 * Return: BENAR jika valid
 */
bool_t proses_pid_valid(pid_t pid)
{
    if (pid == 0) {
        return SALAH;
    }
    
    if (pid >= PID_MAX) {
        return SALAH;
    }
    
    return BENAR;
}

/*
 * proses_adalah_kernel
 * --------------------
 * Cek apakah proses adalah kernel process.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: BENAR jika kernel process
 */
bool_t proses_adalah_kernel(proses_t *proses)
{
    if (proses == NULL) {
        return SALAH;
    }
    
    return (proses->flags & PROSES_FLAG_KERNEL) ? BENAR : SALAH;
}

/*
 * proses_punya_children
 * ---------------------
 * Cek apakah proses memiliki children.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: BENAR jika memiliki children
 */
bool_t proses_punya_children(proses_t *proses)
{
    if (proses == NULL) {
        return SALAH;
    }
    
    return (proses->children != NULL) ? BENAR : SALAH;
}

/*
 * proses_punya_zombie
 * -------------------
 * Cek apakah proses memiliki zombie children.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: BENAR jika memiliki zombie children
 */
bool_t proses_punya_zombie(proses_t *proses)
{
    if (proses == NULL) {
        return SALAH;
    }
    
    return (proses->zombie_children > 0) ? BENAR : SALAH;
}

/*
 * proses_runnable
 * ---------------
 * Cek apakah proses dalam status runnable.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: BENAR jika runnable
 */
bool_t proses_runnable(proses_t *proses)
{
    if (proses == NULL) {
        return SALAH;
    }
    
    return (proses->status == PROSES_STATUS_SIAP ||
            proses->status == PROSES_STATUS_JALAN) ? BENAR : SALAH;
}

/*
 * thread_runnable
 * ---------------
 * Cek apakah thread dalam status runnable.
 *
 * Parameter:
 *   thread - Pointer ke thread
 *
 * Return: BENAR jika runnable
 */
bool_t thread_runnable(thread_t *thread)
{
    if (thread == NULL) {
        return SALAH;
    }
    
    return (thread->status == THREAD_STATUS_SIAP ||
            thread->status == THREAD_STATUS_JALAN) ? BENAR : SALAH;
}

/*
 * dapatkan_quantum
 * ----------------
 * Dapatkan quantum berdasarkan prioritas.
 *
 * Parameter:
 *   prioritas - Level prioritas
 *
 * Return: Nilai quantum
 */
tak_bertanda32_t dapatkan_quantum(tak_bertanda32_t prioritas)
{
    switch (prioritas) {
        case PRIORITAS_KRITIS:
        case PRIORITAS_REALTIME:
            return QUANTUM_REALTIME;
        case PRIORITAS_TINGGI:
            return QUANTUM_TINGGI;
        case PRIORITAS_NORMAL:
            return QUANTUM_NORMAL;
        case PRIORITAS_RENDAH:
        case PRIORITAS_IDLE:
        default:
            return QUANTUM_RENDAH;
    }
}

/*
 * dapatkan_runqueue_idx
 * ---------------------
 * Dapatkan index runqueue berdasarkan prioritas.
 *
 * Parameter:
 *   prioritas - Level prioritas
 *
 * Return: Index runqueue (0-7)
 */
tak_bertanda32_t dapatkan_runqueue_idx(tak_bertanda32_t prioritas)
{
    switch (prioritas) {
        case PRIORITAS_KRITIS:
        case PRIORITAS_REALTIME:
            return 0;
        case PRIORITAS_TINGGI:
            return 1;
        case PRIORITAS_NORMAL:
            return 2;
        case PRIORITAS_RENDAH:
            return 3;
        case PRIORITAS_IDLE:
            return 4;
        default:
            return 5;
    }
}

/*
 * proses_dapat_statistik
 * ----------------------
 * Dapatkan statistik proses.
 *
 * Parameter:
 *   created  - Pointer untuk jumlah proses dibuat
 *   exited   - Pointer untuk jumlah proses exit
 *   switches - Pointer untuk jumlah context switch
 */
void proses_dapat_statistik(tak_bertanda64_t *created,
                            tak_bertanda64_t *exited,
                            tak_bertanda64_t *switches)
{
    if (created != NULL) {
        *created = g_statistik.total_dibuat;
    }
    
    if (exited != NULL) {
        *exited = g_statistik.total_keluar;
    }
    
    if (switches != NULL) {
        *switches = g_statistik.context_switches;
    }
}

/*
 * proses_print_info
 * -----------------
 * Print informasi proses.
 *
 * Parameter:
 *   pid - Process ID
 */
void proses_print_info(pid_t pid)
{
    proses_t *proses;
    const char *status_str;
    const char *prio_str;
    tak_bertanda32_t uptime;
    
    proses = proses_cari(pid);
    if (proses == NULL) {
        kernel_printf("[PROSES] Proses PID %lu tidak ditemukan\n",
                      (tak_bertanda64_t)pid);
        return;
    }
    
    /* Status string */
    switch (proses->status) {
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
        case PROSES_STATUS_STOP:
            status_str = "Stop";
            break;
        case PROSES_STATUS_BLOCK:
            status_str = "Block";
            break;
        default:
            status_str = "Unknown";
            break;
    }
    
    /* Prioritas string */
    switch (proses->prioritas) {
        case PRIORITAS_IDLE:
            prio_str = "Idle";
            break;
        case PRIORITAS_RENDAH:
            prio_str = "Rendah";
            break;
        case PRIORITAS_NORMAL:
            prio_str = "Normal";
            break;
        case PRIORITAS_TINGGI:
            prio_str = "Tinggi";
            break;
        case PRIORITAS_REALTIME:
            prio_str = "Realtime";
            break;
        case PRIORITAS_KRITIS:
            prio_str = "Kritis";
            break;
        default:
            prio_str = "Unknown";
            break;
    }
    
    uptime = hal_timer_get_ticks() - proses->start_time;
    
    kernel_printf("\n[PROSES] Informasi Proses:\n");
    kernel_printf("========================================\n");
    kernel_printf("  PID:        %lu\n", (tak_bertanda64_t)proses->pid);
    kernel_printf("  PPID:       %lu\n", (tak_bertanda64_t)proses->ppid);
    kernel_printf("  Nama:       %s\n", proses->nama);
    kernel_printf("  Status:     %s\n", status_str);
    kernel_printf("  Prioritas:  %s\n", prio_str);
    kernel_printf("  Nice:       %ld\n", (tanda64_t)proses->nice);
    kernel_printf("  Threads:    %lu\n", (tak_bertanda64_t)proses->thread_count);
    kernel_printf("  FD Count:   %lu\n", (tak_bertanda64_t)proses->fd_count);
    kernel_printf("  CPU Time:   %lu ticks\n", proses->cpu_time);
    kernel_printf("  Uptime:     %lu ticks\n", uptime);
    kernel_printf("  CWD:        %s\n", proses->cwd);
    
    if (proses->flags & PROSES_FLAG_KERNEL) {
        kernel_printf("  Type:       Kernel Process\n");
    }
    
    if (proses->status == PROSES_STATUS_ZOMBIE) {
        kernel_printf("  Exit Code:  %ld\n", (tanda64_t)proses->exit_code);
    }
    
    kernel_printf("========================================\n");
}

/*
 * proses_print_list
 * -----------------
 * Print list semua proses.
 */
void proses_print_list(void)
{
    proses_t *proses;
    const char *status_str;
    tak_bertanda32_t zombie_count;
    
    zombie_count = proses_dapat_jumlah_zombie();
    
    kernel_printf("\n[PROSES] Daftar Proses:\n");
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_printf("  PID   PPID  STATUS    PRIO  THR  FD  NAMA\n");
    kernel_printf("----------------------------------------");
    kernel_printf("----------------------------------------\n");
    
    proses = g_proses_list;
    
    while (proses != NULL) {
        switch (proses->status) {
            case PROSES_STATUS_BELUM:
                status_str = "Belum ";
                break;
            case PROSES_STATUS_SIAP:
                status_str = "Siap  ";
                break;
            case PROSES_STATUS_JALAN:
                status_str = "Jalan ";
                break;
            case PROSES_STATUS_TUNGGU:
                status_str = "Tunggu";
                break;
            case PROSES_STATUS_ZOMBIE:
                status_str = "Zombie";
                break;
            case PROSES_STATUS_STOP:
                status_str = "Stop  ";
                break;
            case PROSES_STATUS_BLOCK:
                status_str = "Block ";
                break;
            default:
                status_str = "Unk   ";
                break;
        }
        
        kernel_printf("  %5lu %5lu %s  %c    %3lu %3lu  %s\n",
                      (tak_bertanda64_t)proses->pid,
                      (tak_bertanda64_t)proses->ppid,
                      status_str,
                      proses->prioritas == PRIORITAS_REALTIME ? 'R' :
                      proses->prioritas == PRIORITAS_TINGGI ? 'T' :
                      proses->prioritas == PRIORITAS_NORMAL ? 'N' :
                      proses->prioritas == PRIORITAS_RENDAH ? 'L' : 'I',
                      (tak_bertanda64_t)proses->thread_count,
                      (tak_bertanda64_t)proses->fd_count,
                      proses->nama);
        
        proses = proses->next;
    }
    
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_printf("Total: %lu proses aktif, %lu zombie\n",
                  (tak_bertanda64_t)g_proses_count,
                  (tak_bertanda64_t)zombie_count);
    kernel_printf("\n");
}

/*
 * proses_print_tree
 * -----------------
 * Print tree proses.
 */
void proses_print_tree(void)
{
    /* Helper untuk print tree - simplified version */
    kernel_printf("\n[PROSES] Pohon Proses:\n");
    kernel_printf("========================================\n");
    kernel_printf("PID_KERNEL (0) - kernel\n");
    kernel_printf("  |-- PID_IDLE (2) - idle\n");
    
    /* Cari semua child dari kernel */
    proses_t *child;
    tak_bertanda32_t count = 0;
    
    if (g_proses_kernel != NULL) {
        child = g_proses_kernel->children;
        while (child != NULL) {
            if (child->pid != PID_IDLE) {
                kernel_printf("  |-- PID %lu - %s\n",
                              (tak_bertanda64_t)child->pid,
                              child->nama);
                count++;
            }
            child = child->sibling;
        }
    }
    
    kernel_printf("========================================\n");
    kernel_printf("Total child kernel: %lu\n", (tak_bertanda64_t)count);
}
