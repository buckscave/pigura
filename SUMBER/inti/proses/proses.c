/*
 * PIGURA OS - PROSES.C
 * --------------------
 * Implementasi manajemen proses kernel.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk mengelola
 * proses, termasuk pembuatan, penghancuran, dan pencarian proses.
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

/* Maximum processes */
#define MAX_PROSES              CONFIG_MAKS_PROSES

/* Maximum threads per process */
#define MAX_THREAD_PER_PROSES   CONFIG_MAKS_THREAD_PER_PROSES

/* Process magic */
#define PROSES_MAGIC            0x50524F43  /* "PROC" */

/* Process flags */
#define PROSES_FLAG_NONE        0x00
#define PROSES_FLAG_KERNEL      0x01
#define PROSES_FLAG_ZOMBIE      0x02
#define PROSES_FLAG_STOPPED     0x04
#define PROSES_FLAG_TRACED      0x08

/* Stack size */
#define PROSES_STACK_SIZE       CONFIG_STACK_USER

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Thread structure */
typedef struct thread {
    tak_bertanda32_t tid;            /* Thread ID */
    tak_bertanda32_t pid;            /* Parent process ID */
    struct thread *next;             /* Thread berikutnya */
    struct thread *prev;             /* Thread sebelumnya */
    void *stack;                     /* Stack pointer */
    ukuran_t stack_size;             /* Ukuran stack */
    tak_bertanda32_t flags;          /* Flag thread */
    proses_status_t status;          /* Status thread */
    tanda32_t exit_code;             /* Exit code */
    void *context;                   /* Context pointer */
} thread_t;

/* Process structure */
typedef struct proses {
    tak_bertanda32_t magic;          /* Magic number */
    pid_t pid;                       /* Process ID */
    pid_t ppid;                      /* Parent process ID */
    gid_t gid;                       /* Group ID */
    uid_t uid;                       /* User ID */
    char nama[MAKS_NAMA_PROSES];     /* Nama proses */
    tak_bertanda32_t flags;          /* Flag proses */
    proses_status_t status;          /* Status proses */
    prioritas_t prioritas;           /* Prioritas */
    tanda32_t exit_code;             /* Exit code */

    /* Memory */
    vm_descriptor_t *vm;             /* Virtual memory descriptor */
    alamat_virtual_t entry_point;    /* Entry point */

    /* Threads */
    thread_t *threads;               /* List thread */
    tak_bertanda32_t thread_count;   /* Jumlah thread */
    thread_t *main_thread;           /* Main thread */

    /* File descriptors */
    tak_bertanda32_t fd_count;       /* Jumlah FD */

    /* Scheduling */
    tak_bertanda64_t cpu_time;       /* CPU time used */
    tak_bertanda64_t start_time;     /* Start time */
    tak_bertanda32_t quantum;        /* Quantum remaining */

    /* List pointers */
    struct proses *next;             /* Proses berikutnya */
    struct proses *prev;             /* Proses sebelumnya */

    /* Children */
    struct proses *children;         /* List child process */
    struct proses *sibling;          /* Sibling process */

    /* Wait queue */
    tak_bertanda32_t wait_count;     /* Jumlah child zombie */
} proses_t;

/* Process table */
typedef struct {
    proses_t *proses_list;           /* List semua proses */
    proses_t *proses_free;           /* Free proses descriptors */
    proses_t *zombie_list;           /* List zombie proses */
    tak_bertanda32_t proses_count;   /* Jumlah proses aktif */
    tak_bertanda32_t proses_next_id; /* PID berikutnya */
    spinlock_t lock;                 /* Lock untuk thread safety */
} proses_table_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Process table */
static proses_t proses_pool[MAX_PROSES];
static thread_t thread_pool[MAX_PROSES * MAX_THREAD_PER_PROSES];
static thread_t *thread_free = NULL;

/* Process table state */
static proses_table_t proses_table = {0};

/* Kernel process */
static proses_t *kernel_proses = NULL;

/* Idle process */
static proses_t *idle_proses = NULL;

/* Status */
static bool_t proses_initialized = SALAH;

/* Statistik */
static struct {
    tak_bertanda64_t total_created;
    tak_bertanda64_t total_exited;
    tak_bertanda64_t context_switches;
} proses_stats = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * thread_pool_init
 * ----------------
 * Inisialisasi thread pool.
 */
static void thread_pool_init(void)
{
    tak_bertanda32_t i;

    thread_free = NULL;

    for (i = 0; i < MAX_PROSES * MAX_THREAD_PER_PROSES; i++) {
        thread_pool[i].tid = i + 1;
        thread_pool[i].pid = 0;
        thread_pool[i].next = thread_free;
        thread_pool[i].prev = NULL;
        thread_free = &thread_pool[i];
    }
}

/*
 * thread_alloc
 * ------------
 * Alokasikan deskriptor thread.
 *
 * Return: Pointer ke thread, atau NULL
 */
static thread_t *thread_alloc(void)
{
    thread_t *thread;

    if (thread_free == NULL) {
        return NULL;
    }

    thread = thread_free;
    thread_free = thread->next;

    if (thread_free != NULL) {
        thread_free->prev = NULL;
    }

    thread->next = NULL;
    thread->prev = NULL;
    thread->pid = 0;
    thread->stack = NULL;
    thread->stack_size = 0;
    thread->flags = PROSES_FLAG_NONE;
    thread->status = PROSES_STATUS_BELUM;
    thread->exit_code = 0;
    thread->context = NULL;

    return thread;
}

/*
 * thread_free_desc
 * ----------------
 * Bebaskan deskriptor thread.
 *
 * Parameter:
 *   thread - Pointer ke thread
 */
static void thread_free_desc(thread_t *thread)
{
    if (thread == NULL) {
        return;
    }

    thread->next = thread_free;
    thread->prev = NULL;

    if (thread_free != NULL) {
        thread_free->prev = thread;
    }

    thread_free = thread;
}

/*
 * proses_pool_init
 * ----------------
 * Inisialisasi process pool.
 */
static void proses_pool_init(void)
{
    tak_bertanda32_t i;

    proses_table.proses_list = NULL;
    proses_table.proses_free = NULL;
    proses_table.zombie_list = NULL;
    proses_table.proses_count = 0;
    proses_table.proses_next_id = 1;

    for (i = 0; i < MAX_PROSES; i++) {
        proses_pool[i].magic = PROSES_MAGIC;
        proses_pool[i].pid = 0;
        proses_pool[i].next = proses_table.proses_free;
        proses_pool[i].prev = NULL;
        proses_table.proses_free = &proses_pool[i];
    }
}

/*
 * proses_alloc
 * ------------
 * Alokasikan deskriptor proses.
 *
 * Return: Pointer ke proses, atau NULL
 */
static proses_t *proses_alloc(void)
{
    proses_t *proses;

    if (proses_table.proses_free == NULL) {
        return NULL;
    }

    proses = proses_table.proses_free;
    proses_table.proses_free = proses->next;

    if (proses_table.proses_free != NULL) {
        proses_table.proses_free->prev = NULL;
    }

    /* Assign PID */
    proses->pid = proses_table.proses_next_id++;

    /* Wrap PID */
    if (proses_table.proses_next_id >= MAX_PROSES) {
        proses_table.proses_next_id = 1;
    }

    /* Clear fields */
    proses->ppid = 0;
    proses->gid = 0;
    proses->uid = 0;
    proses->nama[0] = '\0';
    proses->flags = PROSES_FLAG_NONE;
    proses->status = PROSES_STATUS_BELUM;
    proses->prioritas = PRIORITAS_NORMAL;
    proses->exit_code = 0;
    proses->vm = NULL;
    proses->entry_point = 0;
    proses->threads = NULL;
    proses->thread_count = 0;
    proses->main_thread = NULL;
    proses->fd_count = 0;
    proses->cpu_time = 0;
    proses->start_time = 0;
    proses->quantum = QUANTUM_DEFAULT;
    proses->next = NULL;
    proses->prev = NULL;
    proses->children = NULL;
    proses->sibling = NULL;
    proses->wait_count = 0;

    return proses;
}

/*
 * proses_free_desc
 * ----------------
 * Bebaskan deskriptor proses.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
static void proses_free_desc(proses_t *proses)
{
    if (proses == NULL) {
        return;
    }

    proses->magic = PROSES_MAGIC;
    proses->pid = 0;
    proses->next = proses_table.proses_free;
    proses->prev = NULL;

    if (proses_table.proses_free != NULL) {
        proses_table.proses_free->prev = proses;
    }

    proses_table.proses_free = proses;
}

/*
 * proses_add_to_list
 * ------------------
 * Tambahkan proses ke list.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
static void proses_add_to_list(proses_t *proses)
{
    if (proses == NULL) {
        return;
    }

    proses->next = proses_table.proses_list;
    proses->prev = NULL;

    if (proses_table.proses_list != NULL) {
        proses_table.proses_list->prev = proses;
    }

    proses_table.proses_list = proses;
    proses_table.proses_count++;
}

/*
 * proses_remove_from_list
 * -----------------------
 * Hapus proses dari list.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
static void proses_remove_from_list(proses_t *proses)
{
    if (proses == NULL) {
        return;
    }

    if (proses->prev != NULL) {
        proses->prev->next = proses->next;
    } else {
        proses_table.proses_list = proses->next;
    }

    if (proses->next != NULL) {
        proses->next->prev = proses->prev;
    }

    proses->next = NULL;
    proses->prev = NULL;
    proses_table.proses_count--;
}

/*
 * proses_add_child
 * ----------------
 * Tambahkan child proses.
 *
 * Parameter:
 *   parent - Pointer ke parent proses
 *   child - Pointer ke child proses
 */
static void proses_add_child(proses_t *parent, proses_t *child)
{
    if (parent == NULL || child == NULL) {
        return;
    }

    child->sibling = parent->children;
    parent->children = child;
}

/*
 * proses_remove_child
 * -------------------
 * Hapus child proses.
 *
 * Parameter:
 *   parent - Pointer ke parent proses
 *   child - Pointer ke child proses
 */
static void proses_remove_child(proses_t *parent, proses_t *child)
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
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
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
    if (proses_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Inisialisasi process table */
    proses_pool_init();
    thread_pool_init();

    /* Init lock */
    spinlock_init(&proses_table.lock);

    /* Reset stats */
    kernel_memset(&proses_stats, 0, sizeof(proses_stats));

    /* Buat kernel process */
    kernel_proses = proses_alloc();
    if (kernel_proses == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    kernel_proses->pid = PID_KERNEL;
    kernel_proses->ppid = 0;
    kernel_proses->gid = 0;
    kernel_proses->uid = 0;
    kernel_strncpy(kernel_proses->nama, "kernel", MAKS_NAMA_PROSES - 1);
    kernel_proses->flags = PROSES_FLAG_KERNEL;
    kernel_proses->status = PROSES_STATUS_JALAN;
    kernel_proses->prioritas = PRIORITAS_REALTIME;

    /* Kernel process tidak memiliki VM descriptor terpisah */
    kernel_proses->vm = NULL;

    /* Set start time */
    kernel_proses->start_time = hal_timer_get_ticks();

    proses_add_to_list(kernel_proses);

    /* Buat idle process */
    idle_proses = proses_alloc();
    if (idle_proses == NULL) {
        return STATUS_MEMORI_HABIS;
    }

    idle_proses->pid = PID_IDLE;
    idle_proses->ppid = PID_KERNEL;
    kernel_strncpy(idle_proses->nama, "idle", MAKS_NAMA_PROSES - 1);
    idle_proses->flags = PROSES_FLAG_KERNEL;
    idle_proses->status = PROSES_STATUS_SIAP;
    idle_proses->prioritas = PRIORITAS_RENDAH;
    idle_proses->start_time = hal_timer_get_ticks();

    proses_add_to_list(idle_proses);
    proses_add_child(kernel_proses, idle_proses);

    /* Set current process */
    g_proses_sekarang = kernel_proses;

    proses_initialized = BENAR;

    kernel_printf("[PROSES] Process manager initialized\n");
    kernel_printf("         Kernel PID: %lu, Idle PID: %lu\n",
                  kernel_proses->pid, idle_proses->pid);

    return STATUS_BERHASIL;
}

/*
 * proses_create
 * -------------
 * Buat proses baru.
 *
 * Parameter:
 *   nama - Nama proses
 *   ppid - Parent process ID
 *   flags - Flag proses
 *
 * Return: PID proses baru, atau -1 jika gagal
 */
pid_t proses_create(const char *nama, pid_t ppid, tak_bertanda32_t flags)
{
    proses_t *proses;
    proses_t *parent;
    thread_t *main_thread;
    vm_descriptor_t *vm;

    if (!proses_initialized) {
        return PID_INVALID;
    }

    spinlock_kunci(&proses_table.lock);

    /* Alokasikan proses descriptor */
    proses = proses_alloc();
    if (proses == NULL) {
        spinlock_buka(&proses_table.lock);
        return PID_INVALID;
    }

    /* Set nama */
    if (nama != NULL) {
        kernel_strncpy(proses->nama, nama, MAKS_NAMA_PROSES - 1);
        proses->nama[MAKS_NAMA_PROSES - 1] = '\0';
    }

    /* Set parent */
    parent = proses_cari(ppid);
    if (parent != NULL) {
        proses->ppid = ppid;
        proses->gid = parent->gid;
        proses->uid = parent->uid;
    } else {
        proses->ppid = PID_KERNEL;
        proses->gid = 0;
        proses->uid = 0;
    }

    proses->flags = flags;
    proses->status = PROSES_STATUS_BELUM;
    proses->prioritas = PRIORITAS_NORMAL;
    proses->quantum = QUANTUM_DEFAULT;
    proses->start_time = hal_timer_get_ticks();

    /* Buat VM descriptor */
    if (!(flags & PROSES_FLAG_KERNEL)) {
        vm = vm_create_address_space();
        if (vm == NULL) {
            proses_free_desc(proses);
            spinlock_buka(&proses_table.lock);
            return PID_INVALID;
        }

        proses->vm = vm;
    }

    /* Buat main thread */
    main_thread = thread_alloc();
    if (main_thread == NULL) {
        if (proses->vm != NULL) {
            vm_destroy_address_space(proses->vm);
        }

        proses_free_desc(proses);
        spinlock_buka(&proses_table.lock);
        return PID_INVALID;
    }

    main_thread->tid = proses->pid;  /* TID = PID untuk main thread */
    main_thread->pid = proses->pid;
    main_thread->status = PROSES_STATUS_SIAP;
    main_thread->stack_size = PROSES_STACK_SIZE;

    /* Alokasikan stack */
    if (!(flags & PROSES_FLAG_KERNEL)) {
        alamat_virtual_t stack_top;

        /* Alokasikan stack di VM */
        stack_top = vm_map(proses->vm, ALAMAT_STACK_USER,
                           PROSES_STACK_SIZE, 0x03, VMA_FLAG_STACK);

        if (stack_top == 0) {
            thread_free_desc(main_thread);

            if (proses->vm != NULL) {
                vm_destroy_address_space(proses->vm);
            }

            proses_free_desc(proses);
            spinlock_buka(&proses_table.lock);
            return PID_INVALID;
        }

        main_thread->stack = (void *)stack_top;
    }

    proses->main_thread = main_thread;
    proses->thread_count = 1;

    /* Add thread ke proses */
    main_thread->next = proses->threads;
    proses->threads = main_thread;

    /* Add proses ke list */
    proses_add_to_list(proses);

    /* Add sebagai child */
    if (parent != NULL) {
        proses_add_child(parent, proses);
    }

    proses_stats.total_created++;

    spinlock_buka(&proses_table.lock);

    return proses->pid;
}

/*
 * proses_exit
 * -----------
 * Exit proses.
 *
 * Parameter:
 *   pid - Process ID
 *   exit_code - Exit code
 *
 * Return: Status operasi
 */
status_t proses_exit(pid_t pid, tanda32_t exit_code)
{
    proses_t *proses;
    proses_t *parent;
    thread_t *thread;

    if (!proses_initialized) {
        return STATUS_GAGAL;
    }

    spinlock_kunci(&proses_table.lock);

    /* Cari proses */
    proses = proses_cari(pid);
    if (proses == NULL) {
        spinlock_buka(&proses_table.lock);
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Tidak boleh exit kernel process */
    if (proses->flags & PROSES_FLAG_KERNEL) {
        spinlock_buka(&proses_table.lock);
        return STATUS_PARAM_INVALID;
    }

    /* Set status zombie */
    proses->status = PROSES_STATUS_ZOMBIE;
    proses->exit_code = exit_code;
    proses->flags |= PROSES_FLAG_ZOMBIE;

    /* Free threads */
    thread = proses->threads;

    while (thread != NULL) {
        thread_t *next = thread->next;
        thread_free_desc(thread);
        thread = next;
    }

    proses->thread_count = 0;
    proses->threads = NULL;
    proses->main_thread = NULL;

    /* Notify parent */
    parent = proses_cari(proses->ppid);
    if (parent != NULL) {
        parent->wait_count++;
    }

    proses_stats.total_exited++;

    /* Remove dari list aktif */
    proses_remove_from_list(proses);

    /* Add ke zombie list */
    proses->next = proses_table.zombie_list;
    proses->prev = NULL;

    if (proses_table.zombie_list != NULL) {
        proses_table.zombie_list->prev = proses;
    }

    proses_table.zombie_list = proses;

    spinlock_buka(&proses_table.lock);

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

    if (!proses_initialized || pid < 0) {
        return NULL;
    }

    /* Special case untuk kernel process */
    if (pid == PID_KERNEL) {
        return kernel_proses;
    }

    proses = proses_table.proses_list;

    while (proses != NULL) {
        if (proses->pid == pid) {
            return proses;
        }

        proses = proses->next;
    }

    /* Cek zombie list */
    proses = proses_table.zombie_list;

    while (proses != NULL) {
        if (proses->pid == pid) {
            return proses;
        }

        proses = proses->next;
    }

    return NULL;
}

/*
 * proses_get_current
 * ------------------
 * Dapatkan proses saat ini.
 *
 * Return: Pointer ke proses saat ini
 */
proses_t *proses_get_current(void)
{
    return g_proses_sekarang;
}

/*
 * proses_set_current
 * ------------------
 * Set proses saat ini.
 *
 * Parameter:
 *   proses - Pointer ke proses
 */
void proses_set_current(proses_t *proses)
{
    g_proses_sekarang = proses;
}

/*
 * proses_get_kernel
 * -----------------
 * Dapatkan kernel process.
 *
 * Return: Pointer ke kernel process
 */
proses_t *proses_get_kernel(void)
{
    return kernel_proses;
}

/*
 * proses_get_idle
 * ---------------
 * Dapatkan idle process.
 *
 * Return: Pointer ke idle process
 */
proses_t *proses_get_idle(void)
{
    return idle_proses;
}

/*
 * proses_get_count
 * ----------------
 * Dapatkan jumlah proses aktif.
 *
 * Return: Jumlah proses
 */
tak_bertanda32_t proses_get_count(void)
{
    return proses_table.proses_count;
}

/*
 * proses_wait
 * -----------
 * Wait untuk proses child.
 *
 * Parameter:
 *   pid - Process ID (0 untuk any child)
 *   status - Pointer untuk exit status
 *
 * Return: PID child yang exit, atau -1
 */
pid_t proses_wait(pid_t pid, tanda32_t *status)
{
    proses_t *parent;
    proses_t *child;
    proses_t *prev;
    pid_t child_pid;

    if (!proses_initialized) {
        return PID_INVALID;
    }

    parent = proses_get_current();
    if (parent == NULL) {
        return PID_INVALID;
    }

    spinlock_kunci(&proses_table.lock);

    /* Cek apakah ada child zombie */
    if (parent->wait_count == 0) {
        spinlock_buka(&proses_table.lock);
        return PID_INVALID;
    }

    /* Cari child zombie */
    child = proses_table.zombie_list;
    prev = NULL;

    while (child != NULL) {
        if (child->ppid == parent->pid) {
            if (pid == 0 || child->pid == pid) {
                /* Found */
                if (status != NULL) {
                    *status = child->exit_code;
                }

                child_pid = child->pid;

                /* Remove dari zombie list */
                if (prev == NULL) {
                    proses_table.zombie_list = child->next;
                } else {
                    prev->next = child->next;
                }

                /* Remove dari child list */
                proses_remove_child(parent, child);

                /* Free VM */
                if (child->vm != NULL) {
                    vm_destroy_address_space(child->vm);
                }

                /* Free proses descriptor */
                proses_free_desc(child);

                parent->wait_count--;

                spinlock_buka(&proses_table.lock);

                return child_pid;
            }
        }

        prev = child;
        child = child->next;
    }

    spinlock_buka(&proses_table.lock);

    return PID_INVALID;
}

/*
 * proses_kill
 * -----------
 * Kill proses.
 *
 * Parameter:
 *   pid - Process ID
 *   signal - Signal number
 *
 * Return: Status operasi
 */
status_t proses_kill(pid_t pid, tak_bertanda32_t signal)
{
    proses_t *proses;

    if (!proses_initialized) {
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
            return STATUS_TIDAK_DUKUNG;
    }
}

/*
 * proses_set_priority
 * -------------------
 * Set prioritas proses.
 *
 * Parameter:
 *   pid - Process ID
 *   priority - Prioritas baru
 *
 * Return: Status operasi
 */
status_t proses_set_priority(pid_t pid, prioritas_t priority)
{
    proses_t *proses;

    if (!proses_initialized) {
        return STATUS_GAGAL;
    }

    if (priority > PRIORITAS_REALTIME) {
        return STATUS_PARAM_INVALID;
    }

    proses = proses_cari(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    proses->prioritas = priority;

    return STATUS_BERHASIL;
}

/*
 * proses_get_stats
 * ----------------
 * Dapatkan statistik proses.
 *
 * Parameter:
 *   created - Pointer untuk jumlah proses dibuat
 *   exited - Pointer untuk jumlah proses exit
 *   switches - Pointer untuk jumlah context switch
 */
void proses_get_stats(tak_bertanda64_t *created, tak_bertanda64_t *exited,
                      tak_bertanda64_t *switches)
{
    if (created != NULL) {
        *created = proses_stats.total_created;
    }

    if (exited != NULL) {
        *exited = proses_stats.total_exited;
    }

    if (switches != NULL) {
        *switches = proses_stats.context_switches;
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
        kernel_printf("[PROSES] Proses dengan PID %lu tidak ditemukan\n",
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
        default:
            status_str = "Unknown";
            break;
    }

    /* Priority string */
    switch (proses->prioritas) {
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
        default:
            prio_str = "Unknown";
            break;
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

    proses = proses_table.proses_list;

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
            default:
                status_str = "Unk   ";
                break;
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
                  (tak_bertanda64_t)proses_table.proses_count);
    kernel_printf("\n");
}
