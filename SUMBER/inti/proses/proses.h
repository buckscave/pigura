/*
 * PIGURA OS - PROSES.H
 * ---------------------
 * Header utama untuk manajemen proses dan thread.
 *
 * Berkas ini mendefinisikan struktur data, konstanta, dan deklarasi
 * fungsi untuk subsistem proses dengan dukungan penuh 32-bit dan
 * 64-bit pada arsitektur x86, x86_64, ARM, dan ARM64.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi dideklarasikan secara eksplisit
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#ifndef PROSES_PROSES_H
#define PROSES_PROSES_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../types.h"
#include "../konstanta.h"

/*
 * ===========================================================================
 * KONSTANTA PROSES (PROCESS CONSTANTS)
 * ===========================================================================
 */

/* Jumlah maksimum proses */
#define PROSES_MAKS                 CONFIG_MAKS_PROSES

/* Jumlah maksimum thread per proses */
#define THREAD_MAKS_PER_PROSES      CONFIG_MAKS_THREAD

/* Jumlah maksimum file descriptor per proses */
#define FD_MAKS_PER_PROSES          CONFIG_MAKS_FD

/* Jumlah maksimum CPU */
#define CPU_MAKS                    CONFIG_MAKS_CPU

/* Ukuran stack default */
#define STACK_UKURAN_KERNEL         CONFIG_STACK_KERNEL
#define STACK_UKURAN_USER           CONFIG_STACK_USER

/* Ukuran quantum scheduler */
#define QUANTUM_DEFAULT             CONFIG_SCHEDULER_QUANTUM
#define QUANTUM_REALTIME            20
#define QUANTUM_TINGGI              15
#define QUANTUM_NORMAL              10
#define QUANTUM_RENDAH              5
#define QUANTUM_IDLE                1

/* Magic number untuk validasi */
#define PROSES_MAGIC                0x50524F43  /* "PROC" */
#define THREAD_MAGIC                0x54485244  /* "THRD" */
#define FD_MAGIC                    0x46444553  /* "FDUS" */

/* PID khusus */
#define PID_KERNEL                  0
#define PID_INIT                    1
#define PID_IDLE                    2
#define PID_TIDAK_ADA               ((pid_t)-1)

/* TID khusus */
#define TID_TIDAK_ADA               ((tid_t)-1)

/* Nilai invalid */
#define PROSES_INVALID              NULL
#define THREAD_INVALID              NULL

/*
 * ===========================================================================
 * FLAG PROSES (PROCESS FLAGS)
 * ===========================================================================
 */

#define PROSES_FLAG_NONE            0x0000
#define PROSES_FLAG_KERNEL          0x0001  /* Proses kernel */
#define PROSES_FLAG_ZOMBIE          0x0002  /* Proses zombie */
#define PROSES_FLAG_STOPPED         0x0004  /* Proses dihentikan */
#define PROSES_FLAG_TRACED          0x0008  /* Proses ditrace */
#define PROSES_FLAG_DETACHED        0x0010  /* Proses detached */
#define PROSES_FLAG_SESSION         0x0020  /* Session leader */
#define PROSES_FLAG_GROUP           0x0040  /* Group leader */
#define PROSES_FLAG_CONTINUED       0x0080  /* Proses dilanjutkan */
#define PROSES_FLAG_VFORK           0x0100  /* vfork child */
#define PROSES_FLAG_SUID            0x0200  /* Setuid aktif */
#define PROSES_FLAG_SGID            0x0400  /* Setgid aktif */

/*
 * ===========================================================================
 * FLAG THREAD (THREAD FLAGS)
 * ===========================================================================
 */

#define THREAD_FLAG_NONE            0x00
#define THREAD_FLAG_KERNEL          0x01  /* Thread kernel */
#define THREAD_FLAG_DETACHED        0x02  /* Thread detached */
#define THREAD_FLAG_JOINED          0x04  /* Thread di-join */
#define THREAD_FLAG_MAIN            0x08  /* Main thread */
#define THREAD_FLAG_USER            0x10  /* Thread user mode */

/*
 * ===========================================================================
 * STATUS PROSES (PROCESS STATUS)
 * ===========================================================================
 */

/* Status proses - harus sama dengan di types.h */
#ifndef PROSES_STATUS_INVALID
#define PROSES_STATUS_INVALID       0
#define PROSES_STATUS_BELUM         1   /* Baru dibuat */
#define PROSES_STATUS_SIAP          2   /* Siap dijalankan */
#define PROSES_STATUS_JALAN         3   /* Sedang berjalan */
#define PROSES_STATUS_TUNGGU        4   /* Menunggu */
#define PROSES_STATUS_ZOMBIE        5   /* Zombie */
#define PROSES_STATUS_STOP          6   /* Dihentikan */
#define PROSES_STATUS_BLOCK         7   /* Blocked */
#endif

/*
 * ===========================================================================
 * STATUS THREAD (THREAD STATUS)
 * ===========================================================================
 */

#ifndef THREAD_STATUS_INVALID
#define THREAD_STATUS_INVALID       0
#define THREAD_STATUS_BELUM         1
#define THREAD_STATUS_SIAP          2
#define THREAD_STATUS_JALAN         3
#define THREAD_STATUS_TUNGGU        4
#define THREAD_STATUS_STOP          5
#define THREAD_STATUS_BLOCK         6
#define THREAD_STATUS_SLEEP         7
#endif

/*
 * ===========================================================================
 * FLAG SCHEDULER (SCHEDULER FLAGS)
 * ===========================================================================
 */

#define SCHED_FLAG_PREEMPT          0x01
#define SCHED_FLAG_NEED_RESCHED     0x02
#define SCHED_FLAG_IDLE             0x04

/*
 * ===========================================================================
 * FLAG WAIT (WAIT FLAGS)
 * ===========================================================================
 */

#define WAIT_FLAG_NONE              0x00
#define WAIT_FLAG_NOHANG            0x01
#define WAIT_FLAG_UNTRACED          0x02
#define WAIT_FLAG_CONTINUED         0x04
#define WAIT_FLAG_EXITED            0x08

/*
 * ===========================================================================
 * ALASAN BLOCK (BLOCK REASONS)
 * ===========================================================================
 */

#define BLOCK_ALASAN_NONE           0
#define BLOCK_ALASAN_WAIT           1   /* wait() */
#define BLOCK_ALASAN_SLEEP          2   /* sleep() */
#define BLOCK_ALASAN_IO             3   /* I/O operation */
#define BLOCK_ALASAN_VFORK          4   /* vfork() */
#define BLOCK_ALASAN_SIGNAL         5   /* Signal wait */
#define BLOCK_ALASAN_SEMAPHORE      6   /* Semaphore */
#define BLOCK_ALASAN_MUTEX          7   /* Mutex */
#define BLOCK_ALASAN_KONDISI        8   /* Condition variable */

/*
 * ===========================================================================
 * FORWARD DECLARATIONS
 * ===========================================================================
 */

struct proses;
struct thread;
struct cpu_context;
struct vm_descriptor;
struct file_descriptor;

/*
 * ===========================================================================
 * STRUKTUR STATISTIK (STATISTICS STRUCTURES)
 * ===========================================================================
 */

/* Statistik memori proses */
typedef struct {
    ukuran_t total;         /* Total memori */
    ukuran_t kode;          /* Memori kode */
    ukuran_t data;          /* Memori data */
    ukuran_t stack;         /* Memori stack */
    ukuran_t heap;          /* Memori heap */
    ukuran_t shared;        /* Memori shared */
} statistik_memori_t;

/* Statistik CPU proses */
typedef struct {
    tak_bertanda64_t user;      /* Waktu user mode */
    tak_bertanda64_t system;    /* Waktu system mode */
    tak_bertanda64_t idle;      /* Waktu idle */
    tak_bertanda64_t iowait;    /* Waktu menunggu I/O */
} statistik_cpu_t;

/*
 * ===========================================================================
 * STRUKTUR CONTEXT CPU (CPU CONTEXT STRUCTURE)
 * ===========================================================================
 * Struktur ini menyimpan state CPU untuk context switching.
 * Ukuran field berbeda untuk 32-bit dan 64-bit.
 */

#if defined(PIGURA_ARSITEKTUR_64BIT)

/* Context 64-bit untuk x86_64 dan ARM64 */
typedef struct cpu_context {
    /* General purpose registers */
    tak_bertanda64_t rax;       /* Accumulator */
    tak_bertanda64_t rbx;       /* Base */
    tak_bertanda64_t rcx;       /* Counter */
    tak_bertanda64_t rdx;       /* Data */
    tak_bertanda64_t rsi;       /* Source index */
    tak_bertanda64_t rdi;       /* Destination index */
    tak_bertanda64_t rbp;       /* Base pointer */
    tak_bertanda64_t rsp;       /* Stack pointer */
    tak_bertanda64_t r8;        /* Register 8 */
    tak_bertanda64_t r9;        /* Register 9 */
    tak_bertanda64_t r10;       /* Register 10 */
    tak_bertanda64_t r11;       /* Register 11 */
    tak_bertanda64_t r12;       /* Register 12 */
    tak_bertanda64_t r13;       /* Register 13 */
    tak_bertanda64_t r14;       /* Register 14 */
    tak_bertanda64_t r15;       /* Register 15 */
    
    /* Instruction pointer dan flags */
    tak_bertanda64_t rip;       /* Instruction pointer */
    tak_bertanda64_t rflags;    /* Flags register */
    
    /* Segment registers (16-bit) */
    tak_bertanda16_t cs;        /* Code segment */
    tak_bertanda16_t ds;        /* Data segment */
    tak_bertanda16_t es;        /* Extra segment */
    tak_bertanda16_t fs;        /* F segment */
    tak_bertanda16_t gs;        /* G segment */
    tak_bertanda16_t ss;        /* Stack segment */
    
    /* Padding untuk alignment */
    tak_bertanda16_t padding[3];
    
    /* Page directory base */
    tak_bertanda64_t cr3;       /* Page directory */
    
#if defined(ARSITEKTUR_ARM64)
    /* ARM64 specific registers */
    tak_bertanda64_t sp_el0;    /* Stack pointer EL0 */
    tak_bertanda64_t sp_el1;    /* Stack pointer EL1 */
    tak_bertanda64_t elr_el1;   /* Exception link register */
    tak_bertanda64_t spsr_el1;  /* Saved program status */
    tak_bertanda64_t tpidr_el0; /* Thread pointer */
#endif
} cpu_context_t;

#else /* PIGURA_ARSITEKTUR_32BIT */

/* Context 32-bit untuk x86 dan ARM */
typedef struct cpu_context {
    /* General purpose registers */
    tak_bertanda32_t eax;       /* Accumulator */
    tak_bertanda32_t ebx;       /* Base */
    tak_bertanda32_t ecx;       /* Counter */
    tak_bertanda32_t edx;       /* Data */
    tak_bertanda32_t esi;       /* Source index */
    tak_bertanda32_t edi;       /* Destination index */
    tak_bertanda32_t ebp;       /* Base pointer */
    tak_bertanda32_t esp;       /* Stack pointer */
    
    /* Instruction pointer dan flags */
    tak_bertanda32_t eip;       /* Instruction pointer */
    tak_bertanda32_t eflags;    /* Flags register */
    
    /* Segment registers (16-bit) */
    tak_bertanda16_t cs;        /* Code segment */
    tak_bertanda16_t ds;        /* Data segment */
    tak_bertanda16_t es;        /* Extra segment */
    tak_bertanda16_t fs;        /* F segment */
    tak_bertanda16_t gs;        /* G segment */
    tak_bertanda16_t ss;        /* Stack segment */
    
    /* Padding untuk alignment */
    tak_bertanda16_t padding[1];
    
    /* Page directory base */
    tak_bertanda32_t cr3;       /* Page directory */
    
#if defined(ARSITEKTUR_ARM)
    /* ARM specific registers */
    tak_bertanda32_t sp_usr;    /* User mode SP */
    tak_bertanda32_t lr_usr;    /* User mode LR */
    tak_bertanda32_t spsr;      /* Saved program status */
    tak_bertanda32_t cpsr;      /* Current program status */
#endif
} cpu_context_t;

#endif /* PIGURA_ARSITEKTUR_64BIT */

/*
 * ===========================================================================
 * STRUKTUR THREAD (THREAD STRUCTURE)
 * ===========================================================================
 */

typedef struct thread {
    /* Header validasi */
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t tid;           /* Thread ID */
    tak_bertanda32_t pid;           /* Process ID parent */
    
    /* Status dan prioritas */
    tak_bertanda32_t status;        /* Status thread */
    tak_bertanda32_t flags;         /* Flag thread */
    tak_bertanda32_t prioritas;     /* Prioritas */
    tak_bertanda32_t quantum;       /* Quantum remaining */
    
    /* Context CPU */
    struct cpu_context *context;    /* CPU context */
    
    /* Stack */
    void *stack;                    /* Stack pointer */
    ukuran_t stack_size;            /* Ukuran stack */
    
    /* Scheduling */
    tak_bertanda64_t cpu_time;       /* CPU time used */
    tak_bertanda64_t start_time;     /* Start time */
    
    /* Exit status */
    tanda32_t exit_code;            /* Exit code */
    
    /* Linked list */
    struct thread *next;            /* Thread berikutnya */
    struct thread *prev;            /* Thread sebelumnya */
    
    /* Join/Wait */
    struct thread *waiting;         /* Thread yang menunggu */
    void *retval;                   /* Return value */
    
    /* Thread-local storage */
    void *tls;                      /* TLS pointer */
    
    /* Proses parent */
    struct proses *proses;          /* Pointer ke proses */
} thread_t;

/*
 * ===========================================================================
 * STRUKTUR FILE DESCRIPTOR (FILE DESCRIPTOR STRUCTURE)
 * ===========================================================================
 */

typedef struct file_descriptor {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t flags;         /* Flag file */
    tak_bertanda64_t offset;        /* Offset file */
    void *private_data;             /* Data private */
    tak_bertanda32_t ref_count;     /* Reference count */
} file_descriptor_t;

/*
 * ===========================================================================
 * STRUKTUR PROSES (PROCESS STRUCTURE)
 * ===========================================================================
 */

typedef struct proses {
    /* Header validasi */
    tak_bertanda32_t magic;         /* Magic number */
    pid_t pid;                      /* Process ID */
    pid_t ppid;                     /* Parent process ID */
    
    /* Identitas */
    uid_t uid;                      /* User ID */
    gid_t gid;                      /* Group ID */
    uid_t euid;                     /* Effective UID */
    gid_t egid;                     /* Effective GID */
    pid_t pgid;                     /* Process group ID */
    pid_t sid;                      /* Session ID */
    
    /* Nama */
    char nama[32];                  /* Nama proses */
    char cwd[256];                  /* Current working directory */
    char root[256];                 /* Root directory */
    
    /* Status dan flag */
    tak_bertanda32_t status;        /* Status proses */
    tak_bertanda32_t flags;         /* Flag proses */
    tak_bertanda32_t prioritas;     /* Prioritas */
    tanda32_t exit_code;            /* Exit code */
    
    /* Memory management */
    struct vm_descriptor *vm;       /* Virtual memory descriptor */
    alamat_virtual_t entry_point;   /* Entry point */
    tak_bertanda64_t brk_start;     /* Start of heap */
    tak_bertanda64_t brk_current;   /* Current brk */
    tak_bertanda64_t brk_end;       /* End of heap */
    
    /* Thread management */
    thread_t *threads;              /* List thread */
    thread_t *main_thread;          /* Main thread */
    tak_bertanda32_t thread_count;  /* Jumlah thread */
    
    /* File descriptors */
    struct file_descriptor *fd_table[FD_MAKS_PER_PROSES];
    tak_bertanda32_t fd_count;      /* Jumlah FD aktif */
    
    /* Signal handling */
    tak_bertanda32_t signal_mask;   /* Signal mask */
    tak_bertanda32_t signal_pending;/* Signal pending */
    tak_bertanda32_t signal_ignored;/* Signal ignored */
    void *signal_handlers[32];      /* Signal handlers */
    void *signal_trampoline;        /* Signal trampoline */
    tak_bertanda32_t saved_signal_mask;
    
    /* Scheduling */
    tak_bertanda64_t cpu_time;      /* CPU time used */
    tak_bertanda64_t start_time;    /* Start time */
    tak_bertanda64_t end_time;      /* End time */
    tak_bertanda32_t quantum;       /* Quantum remaining */
    
    /* Linked list */
    struct proses *next;            /* Proses berikutnya */
    struct proses *prev;            /* Proses sebelumnya */
    
    /* Child processes */
    struct proses *children;        /* List child process */
    struct proses *sibling;         /* Sibling process */
    struct proses *parent;          /* Parent process */
    
    /* Wait state */
    tak_bertanda32_t wait_state;    /* State untuk wait */
    pid_t wait_pid;                 /* PID yang ditunggu */
    tak_bertanda32_t wait_options;  /* Option wait */
    tak_bertanda32_t zombie_children;/* Jumlah child zombie */
    
    /* vfork state */
    pid_t vfork_child;              /* vfork child PID */
    
    /* Statistik */
    statistik_memori_t mem_stat;    /* Statistik memori */
    statistik_cpu_t cpu_stat;       /* Statistik CPU */
} proses_t;

/*
 * ===========================================================================
 * STRUKTUR RUNQUEUE (RUNQUEUE STRUCTURE)
 * ===========================================================================
 */

typedef struct {
    proses_t *head;                 /* Proses pertama */
    proses_t *tail;                 /* Proses terakhir */
    tak_bertanda32_t count;         /* Jumlah proses */
} runqueue_t;

/*
 * ===========================================================================
 * STRUKTUR SCHEDULER (SCHEDULER STRUCTURE)
 * ===========================================================================
 */

#define SCHEDULER_ANTRIAN_JUMLAH    4   /* Realtime, High, Normal, Low */

typedef struct {
    /* Antrian per prioritas */
    runqueue_t antrian[SCHEDULER_ANTRIAN_JUMLAH];
    
    /* Statistik */
    tak_bertanda32_t total_proses;      /* Total proses runnable */
    tak_bertanda32_t tick_count;        /* Counter tick */
    tak_bertanda32_t context_switches;  /* Jumlah context switch */
    
    /* Boost untuk anti-starvation */
    tak_bertanda32_t last_boost;        /* Tick boost terakhir */
    
    /* Lock */
    spinlock_t lock;                    /* Lock untuk thread safety */
    
    /* Flags */
    tak_bertanda32_t flags;             /* Flag scheduler */
    
    /* Idle process */
    proses_t *idle_proses;              /* Pointer ke idle proses */
} scheduler_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI PROSES (PROCESS FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t proses_init(void);
status_t proses_diinit(void);

/* Buat dan hapus proses */
pid_t proses_buat(const char *nama, pid_t ppid, tak_bertanda32_t flags);
status_t proses_hapus(pid_t pid);

/* Cari dan dapatkan proses */
proses_t *proses_cari(pid_t pid);
proses_t *proses_dapat_saat_ini(void);
proses_t *proses_dapat_kernel(void);
proses_t *proses_dapat_idle(void);

/* Set proses */
void proses_set_saat_ini(proses_t *proses);

/* Jumlah proses */
tak_bertanda32_t proses_dapat_jumlah(void);

/* Exit proses */
status_t proses_exit(pid_t pid, tanda32_t exit_code);

/* Kill proses */
status_t proses_kill(pid_t pid, tak_bertanda32_t signal);

/* Prioritas */
status_t proses_set_prioritas(pid_t pid, tak_bertanda32_t prioritas);
tak_bertanda32_t proses_dapat_prioritas(pid_t pid);

/* Child process */
void proses_tambah_child(proses_t *parent, proses_t *child);
void proses_hapus_child(proses_t *parent, proses_t *child);

/* List management */
void proses_tambah_ke_list(proses_t *proses);
void proses_hapus_dari_list(proses_t *proses);

/* Statistik */
void proses_dapat_statistik(tak_bertanda64_t *created, tak_bertanda64_t *exited,
                            tak_bertanda64_t *switches);

/* Print */
void proses_print_info(pid_t pid);
void proses_print_list(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI THREAD (THREAD FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t thread_init(void);

/* Buat dan hapus thread */
tak_bertanda32_t thread_buat(tak_bertanda32_t pid,
                            void (*entry)(void *), void *arg,
                            tak_bertanda32_t flags);
void thread_exit(void *retval);
status_t thread_join(tak_bertanda32_t tid, void **retval);
status_t thread_detach(tak_bertanda32_t tid);

/* Thread management */
void thread_yield(void);
tak_bertanda32_t thread_self(void);

/* Prioritas thread */
status_t thread_set_prioritas(tak_bertanda32_t tid,
                              tak_bertanda32_t prioritas);
tak_bertanda32_t thread_dapat_prioritas(tak_bertanda32_t tid);

/* Alokasi thread */
thread_t *thread_alokasi(void);
void thread_bebaskan(thread_t *thread);

/* Stack */
void *thread_alokasi_stack(ukuran_t size, bool_t is_user);
void thread_bebaskan_stack(void *stack, ukuran_t size, bool_t is_user);

/* Statistik */
void thread_dapat_statistik(tak_bertanda64_t *created,
                            tak_bertanda64_t *exited,
                            tak_bertanda64_t *active);

/* Print */
void thread_print_info(tak_bertanda32_t tid);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI SCHEDULER (SCHEDULER FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t scheduler_init(void);

/* Tambah dan hapus proses */
status_t scheduler_tambah_proses(proses_t *proses);
status_t scheduler_hapus_proses(proses_t *proses);

/* Schedule */
void scheduler_tick(void);
void scheduler_schedule(void);
status_t scheduler_yield(void);

/* Block dan unblock */
status_t scheduler_block(tak_bertanda32_t reason);
status_t scheduler_unblock(proses_t *proses);

/* Prioritas */
status_t scheduler_set_prioritas(proses_t *proses,
                                 tak_bertanda32_t prioritas);

/* Load */
tak_bertanda32_t scheduler_dapat_beban(void);

/* Check */
bool_t scheduler_perlu_resched(void);

/* Statistik */
void scheduler_dapat_statistik(tak_bertanda64_t *switches,
                               tak_bertanda64_t *preemptions,
                               tak_bertanda64_t *yields,
                               tak_bertanda64_t *timeouts);

/* Print */
void scheduler_print_stats(void);
void scheduler_print_queues(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI CONTEXT (CONTEXT FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t context_init(void);

/* Buat dan hapus context */
void *context_buat(alamat_virtual_t entry_point, void *stack_ptr,
                   bool_t is_user);
void context_hancurkan(void *ctx);

/* Context switch */
void context_switch(proses_t *old_proses, proses_t *new_proses);
void context_switch_to(void *ctx);

/* Set context */
status_t context_set_entry(void *ctx, alamat_virtual_t entry_point);
status_t context_set_stack(void *ctx, void *stack_ptr);
status_t context_set_cr3(void *ctx, tak_bertanda64_t cr3);
void context_set_return(void *ctx, tanda64_t retval);

/* Get context */
void *context_dapat_stack(void *ctx);
alamat_virtual_t context_dapat_entry(void *ctx);

/* Setup context */
status_t context_setup_user(void *ctx, alamat_virtual_t entry_point,
                            alamat_virtual_t stack_top,
                            tak_bertanda32_t argc, tak_bertanda32_t argv);
status_t context_setup_kernel(void *ctx, alamat_virtual_t entry_point,
                              void *arg);

/* Duplikasi context */
void *context_dup(void *ctx);

/* Return ke user */
void context_return_to_user(void *ctx);

/* Statistik */
void context_dapat_statistik(tak_bertanda64_t *total,
                             tak_bertanda64_t *kernel_to_user,
                             tak_bertanda64_t *user_to_kernel,
                             tak_bertanda64_t *user_to_user);

/* Print */
void context_print_info(void *ctx);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI FORK (FORK FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t fork_init(void);

/* Fork */
pid_t do_fork(tak_bertanda32_t flags, void *stack);

/* System call */
pid_t sys_fork(void);
pid_t sys_vfork(void);
pid_t sys_clone(tak_bertanda32_t flags, void *stack,
                pid_t *parent_tid, void *child_tls,
                pid_t *child_tid);

/* Statistik */
void fork_dapat_statistik(tak_bertanda64_t *total,
                          tak_bertanda64_t *successful,
                          tak_bertanda64_t *failed,
                          tak_bertanda64_t *vforks);

/* Print */
void fork_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI EXEC (EXEC FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t exec_init(void);

/* Exec */
status_t do_exec(const char *path, char *const argv[],
                char *const envp[]);

/* System call */
tanda32_t sys_execve(const char *path, char *const argv[],
                     char *const envp[]);
tanda32_t sys_execvp(const char *file, char *const argv[]);

/* Statistik */
void exec_dapat_statistik(tak_bertanda64_t *total,
                          tak_bertanda64_t *successful,
                          tak_bertanda64_t *failed,
                          tak_bertanda64_t *elf);

/* Print */
void exec_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI EXIT (EXIT FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t exit_init(void);

/* Exit */
void do_exit(tanda32_t exit_code);
void do_exit_group(tanda32_t exit_code);

/* System call */
void sys_exit(tanda32_t exit_code);
void sys_exit_group(tanda32_t exit_code);

/* Exit by signal */
void exit_by_signal(proses_t *proses, tak_bertanda32_t signal,
                    bool_t core);

/* Zombie cleanup */
tak_bertanda32_t exit_cleanup_zombies(tak_bertanda32_t max_count);
tak_bertanda32_t exit_dapat_jumlah_zombie(void);

/* Statistik */
void exit_dapat_statistik(tak_bertanda64_t *total,
                          tak_bertanda64_t *normal,
                          tak_bertanda64_t *signal,
                          tak_bertanda64_t *kernel);

/* Print */
void exit_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI WAIT (WAIT FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t wait_init(void);

/* Wait */
pid_t do_wait(pid_t pid, tanda32_t *status,
              tak_bertanda32_t options, void *rusage);

/* System call */
pid_t sys_waitpid(pid_t pid, tanda32_t *status,
                  tak_bertanda32_t options);
pid_t sys_wait(tanda32_t *status);
tanda32_t sys_waitid(tak_bertanda32_t idtype, pid_t id,
                     void *infop, tak_bertanda32_t options);
pid_t sys_wait4(pid_t pid, tanda32_t *status,
                tak_bertanda32_t options, void *rusage);

/* Wakeup */
void wait_wakeup_parent(proses_t *child);

/* Statistik */
void wait_dapat_statistik(tak_bertanda64_t *total,
                          tak_bertanda64_t *successful,
                          tak_bertanda64_t *timeouts,
                          tak_bertanda64_t *interrupted);

/* Print */
void wait_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI SIGNAL (SIGNAL FUNCTIONS)
 * ===========================================================================
 */

/* Jumlah signal */
#define SIGNAL_JUMLAH           32

/* Nomor signal standar */
#define SIGNAL_HUP              1
#define SIGNAL_INT             2
#define SIGNAL_QUIT            3
#define SIGNAL_ILL             4
#define SIGNAL_TRAP            5
#define SIGNAL_ABRT            6
#define SIGNAL_BUS             7
#define SIGNAL_FPE             8
#define SIGNAL_KILL            9
#define SIGNAL_USR1            10
#define SIGNAL_SEGV            11
#define SIGNAL_USR2            12
#define SIGNAL_PIPE            13
#define SIGNAL_ALRM            14
#define SIGNAL_TERM            15
#define SIGNAL_CHLD            17
#define SIGNAL_CONT            18
#define SIGNAL_STOP            19

/* Inisialisasi */
status_t signal_init(void);

/* Kirim signal */
status_t signal_kirim(pid_t pid, tak_bertanda32_t sig);
status_t signal_kirim_ke_proses(proses_t *target, tak_bertanda32_t sig);

/* Handle signal */
void signal_handle_pending(void);
bool_t signal_ada_pending(void);

/* Handler */
status_t signal_set_handler(tak_bertanda32_t sig, void (*handler)(int),
                            tak_bertanda32_t flags);

/* Mask */
void signal_set_mask(tak_bertanda32_t mask);
void signal_block(tak_bertanda32_t mask);
void signal_unblock(tak_bertanda32_t mask);
tak_bertanda32_t signal_dapat_mask(void);
tak_bertanda32_t signal_dapat_pending_set(void);

/* Setup frame */
void signal_setup_frame(proses_t *proses, tak_bertanda32_t sig,
                        void (*handler)(int));
void signal_restore(proses_t *proses);

/* Nama signal */
const char *signal_dapat_nama(tak_bertanda32_t sig);

/* Statistik */
void signal_dapat_statistik(tak_bertanda64_t *sent,
                            tak_bertanda64_t *delivered,
                            tak_bertanda64_t *ignored,
                            tak_bertanda64_t *dropped);

/* Print */
void signal_print_stats(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI ELF LOADER (ELF LOADER FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t elf_init(void);

/* Load */
status_t elf_load(const char *path, proses_t *proses, void *info);
status_t elf_load_from_memory(const void *buffer, ukuran_t size,
                              proses_t *proses, void *info);

/* Statistik */
void elf_dapat_statistik(tak_bertanda64_t *files,
                         tak_bertanda64_t *segments,
                         tak_bertanda64_t *bytes,
                         tak_bertanda64_t *errors);

/* Print */
void elf_print_stats(void);
void elf_print_info(const char *path);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI TSS (TSS FUNCTIONS) - x86 ONLY
 * ===========================================================================
 */

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)

/* Inisialisasi */
status_t tss_init(void);

/* Get TSS */
void *tss_dapat_saat_ini(void);

/* Set kernel stack */
void tss_set_kernel_stack(tak_bertanda64_t esp0);

/* Set CR3 */
void tss_set_cr3(tak_bertanda64_t cr3);

/* I/O permission */
void tss_set_io_permission(tak_bertanda16_t port, bool_t allow);
void tss_set_io_permission_range(tak_bertanda16_t start,
                                 tak_bertanda32_t count, bool_t allow);
void tss_set_io_bitmap(const tak_bertanda8_t *bitmap);

/* Create/destroy */
void *tss_buat(void);
void tss_hancurkan(void *tss);

/* Save/load state */
void tss_simpan_state(void *tss, const void *state);
void tss_muat_state(const void *tss, void *state);

/* Print */
void tss_print_info(void *tss);

#endif /* ARSITEKTUR_X86 || ARSITEKTUR_X86_64 */

/*
 * ===========================================================================
 * DEKLARASI FUNGSI RING SWITCH (RING SWITCH FUNCTIONS)
 * ===========================================================================
 */

/* Switch ke user mode */
void switch_ke_user_mode(void *entry, void *stack,
                         tak_bertanda32_t argc, tak_bertanda32_t argv);

/* Return ke kernel */
void return_ke_kernel(void);

/* Validasi user pointer */
bool_t validasi_pointer_user(const void *ptr);
bool_t validasi_string_user(const char *str);
bool_t validasi_array_user(const void *arr, ukuran_t elem_size,
                            ukuran_t count);

#endif /* PROSES_PROSES_H */
