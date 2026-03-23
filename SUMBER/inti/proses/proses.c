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
 * - Semua fungsi dideklarasikan secara eksplisit
 *
 * Versi: 2.0
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

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Pool proses statis */
static proses_t g_pool_proses[PROSES_MAKS];

/* Pool thread statis */
static thread_t g_pool_thread[PROSES_MAKS * THREAD_MAKS_PER_PROSES];

/* List proses */
static proses_t *g_proses_list = NULL;
static proses_t *g_proses_free = NULL;
static proses_t *g_zombie_list = NULL;

/* List thread free */
static thread_t *g_thread_free = NULL;

/* Proses khusus */
static proses_t *g_proses_kernel = NULL;
static proses_t *g_proses_idle = NULL;

/* Proses saat ini */
proses_t *g_proses_sekarang = NULL;

/* Jumlah proses */
static tak_bertanda32_t g_proses_count = 0;
static tak_bertanda32_t g_proses_next_pid = 1;

/* Status inisialisasi */
static bool_t g_proses_diinit = SALAH;

/* Lock */
static spinlock_t g_proses_lock;

/* Statistik */
static struct {
    tak_bertanda64_t total_dibuat;
    tak_bertanda64_t total_keluar;
    tak_bertanda64_t context_switches;
} g_statistik = {0, 0, 0};

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

    total = PROSES_MAKS * THREAD_MAKS_PER_PROSES;
    g_thread_free = NULL;

    for (i = 0; i < total; i++) {
        g_pool_thread[i].magic = THREAD_MAGIC;
        g_pool_thread[i].tid = i + 1;
        g_pool_thread[i].pid = 0;
        g_pool_thread[i].status = THREAD_STATUS_INVALID;
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
static thread_t *thread_alokasi_internal(void)
{
    thread_t *thread;

    if (g_thread_free == NULL) {
        return NULL;
    }

    thread = g_thread_free;
    g_thread_free = thread->next;

    if (g_thread_free != NULL) {
        g_thread_free->prev = NULL;
    }

    /* Reset field */
    thread->next = NULL;
    thread->prev = NULL;
    thread->pid = 0;
    thread->proses = NULL;
    thread->context = NULL;
    thread->stack = NULL;
    thread->stack_size = 0;
    thread->flags = THREAD_FLAG_NONE;
    thread->status = THREAD_STATUS_BELUM;
    thread->exit_code = 0;
    thread->waiting = NULL;
    thread->retval = NULL;
    thread->tls = NULL;
    thread->cpu_time = 0;
    thread->start_time = 0;
    thread->prioritas = PRIORITAS_NORMAL;
    thread->quantum = QUANTUM_DEFAULT;

    return thread;
}

/*
 * thread_bebaskan_internal
 * ------------------------
 * Bebaskan deskriptor thread ke pool.
 *
 * Parameter:
 *   thread - Pointer ke thread
 */
static void thread_bebaskan_internal(thread_t *thread)
{
    if (thread == NULL) {
        return;
    }

    thread->magic = THREAD_MAGIC;
    thread->status = THREAD_STATUS_INVALID;
    thread->next = g_thread_free;
    thread->prev = NULL;

    if (g_thread_free != NULL) {
        g_thread_free->prev = thread;
    }

    g_thread_free = thread;
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
    g_proses_next_pid = 1;

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

    if (g_proses_free == NULL) {
        return NULL;
    }

    proses = g_proses_free;
    g_proses_free = proses->next;

    if (g_proses_free != NULL) {
        g_proses_free->prev = NULL;
    }

    /* Assign PID */
    proses->pid = g_proses_next_pid++;

    /* Wrap PID jika mencapai batas */
    if (g_proses_next_pid >= PROSES_MAKS) {
        g_proses_next_pid = 1;
    }

    /* Reset field */
    proses->ppid = 0;
    proses->uid = 0;
    proses->gid = 0;
    proses->euid = 0;
    proses->egid = 0;
    proses->pgid = 0;
    proses->sid = 0;
    proses->nama[0] = '\0';
    proses->cwd[0] = '\0';
    proses->root[0] = '\0';
    proses->flags = PROSES_FLAG_NONE;
    proses->status = PROSES_STATUS_BELUM;
    proses->prioritas = PRIORITAS_NORMAL;
    proses->exit_code = 0;
    proses->vm = NULL;
    proses->entry_point = 0;
    proses->brk_start = 0;
    proses->brk_current = 0;
    proses->brk_end = 0;
    proses->threads = NULL;
    proses->main_thread = NULL;
    proses->thread_count = 0;
    proses->fd_count = 0;
    proses->signal_mask = 0;
    proses->signal_pending = 0;
    proses->signal_ignored = 0;
    proses->cpu_time = 0;
    proses->start_time = 0;
    proses->end_time = 0;
    proses->quantum = QUANTUM_DEFAULT;
    proses->next = NULL;
    proses->prev = NULL;
    proses->children = NULL;
    proses->sibling = NULL;
    proses->parent = NULL;
    proses->wait_state = 0;
    proses->wait_pid = 0;
    proses->wait_options = 0;
    proses->zombie_children = 0;
    proses->vfork_child = 0;

    /* Clear FD table */
    kernel_memset(proses->fd_table, 0, sizeof(proses->fd_table));

    /* Clear signal handlers */
    kernel_memset(proses->signal_handlers, 0, sizeof(proses->signal_handlers));

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

    proses->magic = PROSES_MAGIC;
    proses->pid = 0;
    proses->status = PROSES_STATUS_INVALID;
    proses->next = g_proses_free;
    proses->prev = NULL;

    if (g_proses_free != NULL) {
        g_proses_free->prev = proses;
    }

    g_proses_free = proses;
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
    if (g_proses_diinit) {
        return STATUS_SUDAH_ADA;
    }

    /* Inisialisasi pool */
    proses_pool_init();
    thread_pool_init();

    /* Init lock */
    spinlock_init(&g_proses_lock);

    /* Reset statistik */
    kernel_memset(&g_statistik, 0, sizeof(g_statistik));

    /* Buat proses kernel */
    g_proses_kernel = proses_alokasi_internal();
    if (g_proses_kernel == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    g_proses_kernel->pid = PID_KERNEL;
    g_proses_kernel->ppid = 0;
    g_proses_kernel->gid = 0;
    g_proses_kernel->uid = 0;
    kernel_strncpy(g_proses_kernel->nama, "kernel", NAMA_PROSES_MAKS - 1);
    g_proses_kernel->flags = PROSES_FLAG_KERNEL;
    g_proses_kernel->status = PROSES_STATUS_JALAN;
    g_proses_kernel->prioritas = PRIORITAS_REALTIME;
    g_proses_kernel->quantum = QUANTUM_REALTIME;
    kernel_strncpy(g_proses_kernel->cwd, "/", PATH_MAKS - 1);
    kernel_strncpy(g_proses_kernel->root, "/", PATH_MAKS - 1);

    /* Proses kernel tidak memiliki VM terpisah */
    g_proses_kernel->vm = NULL;

    /* Set waktu mulai */
    g_proses_kernel->start_time = hal_timer_get_ticks();

    /* Tambah ke list */
    proses_tambah_ke_list(g_proses_kernel);

    /* Buat proses idle */
    g_proses_idle = proses_alokasi_internal();
    if (g_proses_idle == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    g_proses_idle->pid = PID_IDLE;
    g_proses_idle->ppid = PID_KERNEL;
    kernel_strncpy(g_proses_idle->nama, "idle", NAMA_PROSES_MAKS - 1);
    g_proses_idle->flags = PROSES_FLAG_KERNEL;
    g_proses_idle->status = PROSES_STATUS_SIAP;
    g_proses_idle->prioritas = PRIORITAS_RENDAH;
    g_proses_idle->quantum = QUANTUM_IDLE;
    g_proses_idle->start_time = hal_timer_get_ticks();
    kernel_strncpy(g_proses_idle->cwd, "/", PATH_MAKS - 1);

    proses_tambah_ke_list(g_proses_idle);
    proses_tambah_child(g_proses_kernel, g_proses_idle);

    /* Set proses saat ini */
    g_proses_sekarang = g_proses_kernel;

    g_proses_diinit = BENAR;

    kernel_printf("[PROSES] Process manager initialized\n");
    kernel_printf("         Kernel PID: %lu, Idle PID: %lu\n",
                  (tak_bertanda64_t)g_proses_kernel->pid,
                  (tak_bertanda64_t)g_proses_idle->pid);

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
    if (nama != NULL) {
        kernel_strncpy(proses->nama, nama, NAMA_PROSES_MAKS - 1);
        proses->nama[NAMA_PROSES_MAKS - 1] = '\0';
    }

    /* Cari parent */
    parent = proses_cari(ppid);
    if (parent != NULL) {
        proses->ppid = ppid;
        proses->gid = parent->gid;
        proses->uid = parent->uid;
        proses->euid = parent->euid;
        proses->egid = parent->egid;
        proses->pgid = parent->pgid;
        proses->sid = parent->sid;
        kernel_strncpy(proses->cwd, parent->cwd, PATH_MAKS - 1);
        kernel_strncpy(proses->root, parent->root, PATH_MAKS - 1);
    } else {
        proses->ppid = PID_KERNEL;
        kernel_strncpy(proses->cwd, "/", PATH_MAKS - 1);
        kernel_strncpy(proses->root, "/", PATH_MAKS - 1);
    }

    proses->flags = flags;
    proses->status = PROSES_STATUS_BELUM;
    proses->prioritas = PRIORITAS_NORMAL;
    proses->quantum = QUANTUM_DEFAULT;
    proses->start_time = hal_timer_get_ticks();

    /* Alokasikan main thread */
    main_thread = thread_alokasi_internal();
    if (main_thread == NULL) {
        proses_bebaskan_internal(proses);
        spinlock_buka(&g_proses_lock);
        return PID_TIDAK_ADA;
    }

    main_thread->pid = proses->pid;
    main_thread->tid = proses->pid;
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

        if (thread->stack != NULL) {
            thread_bebaskan_stack(thread->stack, thread->stack_size,
                                   !(proses->flags & PROSES_FLAG_KERNEL));
        }

        thread_bebaskan_internal(thread);
        thread = next_thread;
    }

    /* Tutup file descriptors */
    for (i = 0; i < FD_MAKS_PER_PROSES; i++) {
        if (proses->fd_table[i] != NULL) {
            /* TODO: Implementasi berkas_tutup */
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

    if (!g_proses_diinit || pid < 0) {
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
 * proses_dapat_saat_ini
 * ---------------------
 * Dapatkan proses saat ini.
 *
 * Return: Pointer ke proses saat ini
 */
proses_t *proses_dapat_saat_ini(void)
{
    return g_proses_sekarang;
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
 * proses_set_saat_ini
 * -------------------
 * Set proses saat ini.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
void proses_set_saat_ini(proses_t *proses)
{
    g_proses_sekarang = proses;
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

    /* Bebaskan threads */
    thread = proses->threads;
    while (thread != NULL) {
        next = thread->next;
        thread->status = THREAD_STATUS_INVALID;
        thread = next;
    }

    proses->thread_count = 0;

    /* Notify parent */
    parent = proses_cari(proses->ppid);
    if (parent != NULL) {
        parent->zombie_children++;
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
            proses->status = PROSES_STATUS_SIAP;
            proses->flags &= ~PROSES_FLAG_STOPPED;
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

    if (prioritas > PRIORITAS_REALTIME) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&g_proses_lock);

    proses = proses_cari(pid);
    if (proses == NULL) {
        spinlock_buka(&g_proses_lock);
        return STATUS_TIDAK_DITEMUKAN;
    }

    proses->prioritas = prioritas;

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
    }
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
    g_proses_count--;
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

    proses = proses_cari(pid);
    if (proses == NULL) {
        kernel_printf("[PROSES] Proses PID %lu tidak ditemukan\n",
                      (tak_bertanda64_t)pid);
        return;
    }

    /* Status string */
    switch (proses->status) {
        case PROSES_STATUS_BELUM:  status_str = "Belum"; break;
        case PROSES_STATUS_SIAP:   status_str = "Siap"; break;
        case PROSES_STATUS_JALAN:  status_str = "Jalan"; break;
        case PROSES_STATUS_TUNGGU: status_str = "Tunggu"; break;
        case PROSES_STATUS_ZOMBIE: status_str = "Zombie"; break;
        case PROSES_STATUS_STOP:   status_str = "Stop"; break;
        default:                   status_str = "Unknown"; break;
    }

    /* Prioritas string */
    switch (proses->prioritas) {
        case PRIORITAS_RENDAH:   prio_str = "Rendah"; break;
        case PRIORITAS_NORMAL:   prio_str = "Normal"; break;
        case PRIORITAS_TINGGI:   prio_str = "Tinggi"; break;
        case PRIORITAS_REALTIME: prio_str = "Realtime"; break;
        default:                 prio_str = "Unknown"; break;
    }

    kernel_printf("\n[PROSES] Informasi Proses:\n");
    kernel_printf("========================================\n");
    kernel_printf("  PID:        %lu\n", (tak_bertanda64_t)proses->pid);
    kernel_printf("  PPID:       %lu\n", (tak_bertanda64_t)proses->ppid);
    kernel_printf("  Nama:       %s\n", proses->nama);
    kernel_printf("  Status:     %s\n", status_str);
    kernel_printf("  Prioritas:  %s\n", prio_str);
    kernel_printf("  Threads:    %lu\n",
                  (tak_bertanda64_t)proses->thread_count);
    kernel_printf("  CPU Time:   %lu ticks\n", proses->cpu_time);
    kernel_printf("  Start Time: %lu ticks\n", proses->start_time);

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

    kernel_printf("\n[PROSES] Daftar Proses:\n");
    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_printf("  PID   PPID  STATUS    PRIO  THREADS  NAMA\n");
    kernel_printf("----------------------------------------");
    kernel_printf("----------------------------------------\n");

    proses = g_proses_list;

    while (proses != NULL) {
        switch (proses->status) {
            case PROSES_STATUS_BELUM:  status_str = "Belum "; break;
            case PROSES_STATUS_SIAP:   status_str = "Siap  "; break;
            case PROSES_STATUS_JALAN:  status_str = "Jalan "; break;
            case PROSES_STATUS_TUNGGU: status_str = "Tunggu"; break;
            case PROSES_STATUS_ZOMBIE: status_str = "Zombie"; break;
            case PROSES_STATUS_STOP:   status_str = "Stop  "; break;
            default:                   status_str = "Unk   "; break;
        }

        kernel_printf("  %5lu %5lu %s  %c    %7lu  %s\n",
                      (tak_bertanda64_t)proses->pid,
                      (tak_bertanda64_t)proses->ppid,
                      status_str,
                      proses->prioritas == PRIORITAS_REALTIME ? 'R' :
                      proses->prioritas == PRIORITAS_TINGGI ? 'T' :
                      proses->prioritas == PRIORITAS_NORMAL ? 'N' : 'L',
                      (tak_bertanda64_t)proses->thread_count,
                      proses->nama);

        proses = proses->next;
    }

    kernel_printf("========================================");
    kernel_printf("========================================\n");
    kernel_printf("Total: %lu proses aktif\n",
                  (tak_bertanda64_t)g_proses_count);
    kernel_printf("\n");
}
