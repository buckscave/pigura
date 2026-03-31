/*
 * PIGURA OS - PROSES.H
 * ---------------------
 * Header utama untuk manajemen proses dan thread.
 *
 * Berkas ini mendefinisikan struktur data, konstanta, dan deklarasi
 * fungsi untuk subsistem proses dengan dukungan penuh 32-bit dan
 * 64-bit pada arsitektur x86, x86_64, ARM, ARMv7, dan ARM64.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi dideklarasikan secara eksplisit
 * - Batas 80 karakter per baris
 *
 * Versi: 3.0
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

/* Batas proses - menggunakan dari konstanta.h */
#define PROSES_MAKS                 MAKS_PROSES
#define THREAD_MAKS_PER_PROSES      MAKS_THREAD_PER_PROSES
#define FD_MAKS_PER_PROSES          MAKS_FD_PER_PROSES
#define CPU_MAKS                    CONFIG_MAKS_CPU

/* Ukuran stack */
#define STACK_UKURAN_KERNEL         UKURAN_STACK_KERNEL
#define STACK_UKURAN_USER           UKURAN_STACK_USER
#define STACK_UKURAN_GUARD          UKURAN_STACK_GUARD

/* Quantum scheduler */
#define QUANTUM_REALTIME            20
#define QUANTUM_TINGGI              15
#define QUANTUM_NORMAL              10
#define QUANTUM_RENDAH              5
#define QUANTUM_IDLE                1

/* Magic number untuk validasi - menggunakan dari konstanta.h */
/* PROSES_MAGIC sudah didefinisikan di konstanta.h */
#ifndef THREAD_MAGIC
#define THREAD_MAGIC                0x54485244  /* "THRD" */
#endif
#ifndef FD_MAGIC
#define FD_MAGIC                    0x46444553  /* "FDUS" */
#endif

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
#define CONTEXT_INVALID             NULL

/*
 * ===========================================================================
 * FLAG PROSES (PROCESS FLAGS)
 * ===========================================================================
 */

#define PROSES_FLAG_NONE            0x0000
#define PROSES_FLAG_KERNEL          0x0001  /* Proses kernel */
#define PROSES_FLAG_DAEMON          0x0002  /* Proses daemon */
#define PROSES_FLAG_TRACED          0x0004  /* Sedang di-trace */
#define PROSES_FLAG_STOPPED         0x0008  /* Sedang stopped */
#define PROSES_FLAG_ZOMBIE          0x0010  /* Status zombie */
#define PROSES_FLAG_DETACHED        0x0020  /* Proses detached */
#define PROSES_FLAG_SESSION         0x0040  /* Session leader */
#define PROSES_FLAG_GROUP           0x0080  /* Group leader */
#define PROSES_FLAG_CONTINUED       0x0100  /* Proses dilanjutkan */
#define PROSES_FLAG_VFORK           0x0200  /* vfork child */
#define PROSES_FLAG_SUID            0x0400  /* Setuid aktif */
#define PROSES_FLAG_SGID            0x0800  /* Setgid aktif */
#define PROSES_FLAG_RUNNING         0x1000  /* Sedang berjalan */
#define PROSES_FLAG_PRIVILEGED      0x2000  /* Proses privileged */

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
#define THREAD_FLAG_SLEEPING        0x20  /* Thread sleeping */
#define THREAD_FLAG_SIGNAL          0x40  /* Signal pending */

/*
 * ===========================================================================
 * STATUS PROSES (PROCESS STATUS)
 * ===========================================================================
 * Status proses didefinisikan di types.h sebagai enum proses_status_t.
 */

/* Makro untuk kompatibilitas */
#ifndef PROSES_STATUS_INVALID
#define PROSES_STATUS_INVALID       0
#define PROSES_STATUS_BELUM         1
#define PROSES_STATUS_SIAP          2
#define PROSES_STATUS_JALAN         3
#define PROSES_STATUS_TUNGGU        4
#define PROSES_STATUS_ZOMBIE        5
#define PROSES_STATUS_STOP          6
#define PROSES_STATUS_BLOCK         7
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
#define THREAD_STATUS_ZOMBIE        8
#endif

/*
 * ===========================================================================
 * FLAG SCHEDULER (SCHEDULER FLAGS)
 * ===========================================================================
 */

#define SCHED_FLAG_NONE             0x00
#define SCHED_FLAG_PREEMPT          0x01
#define SCHED_FLAG_NEED_RESCHED     0x02
#define SCHED_FLAG_IDLE             0x04
#define SCHED_FLAG_RT               0x08

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

/* Status encoding untuk wait */
#define WAIT_STATUS_EXITED          0x00
#define WAIT_STATUS_SIGNALED        0x01
#define WAIT_STATUS_STOPPED         0x02
#define WAIT_STATUS_CONTINUED       0x03
#define WAIT_STATUS_SHIFT           8

/*
 * ===========================================================================
 * ALASAN BLOCK (BLOCK REASONS)
 * ===========================================================================
 */

#define BLOCK_ALASAN_NONE           0
#define BLOCK_ALASAN_WAIT           1   /* wait() */
#define BLOCK_ALASAN_SLEEP          2   /* sleep() */
#define BLOCK_ALASAN_IO             3   /* Operasi I/O */
#define BLOCK_ALASAN_VFORK          4   /* vfork() */
#define BLOCK_ALASAN_SIGNAL         5   /* Menunggu signal */
#define BLOCK_ALASAN_SEMAPHORE      6   /* Semaphore */
#define BLOCK_ALASAN_MUTEX          7   /* Mutex */
#define BLOCK_ALASAN_KONDISI        8   /* Condition variable */
#define BLOCK_ALASAN_PIPE           9   /* Pipe read/write */
#define BLOCK_ALASAN_SOCKET         10  /* Socket operation */

/*
 * ===========================================================================
 * PRIORITAS (PRIORITY)
 * ===========================================================================
 */

#ifndef PRIORITAS_INVALID
#define PRIORITAS_INVALID           0
#define PRIORITAS_IDLE              1
#define PRIORITAS_RENDAH            2
#define PRIORITAS_NORMAL            3
#define PRIORITAS_TINGGI            4
#define PRIORITAS_REALTIME          5
#define PRIORITAS_KRITIS            6
#endif

/* Jumlah level prioritas */
#define PRIORITAS_JUMLAH            7

/* Nilai nice */
#define NICE_MIN                    -20
#define NICE_MAX                    19
#define NICE_DEFAULT                0

/*
 * ===========================================================================
 * SIGNAL CONSTANTS
 * ===========================================================================
 */

#ifndef SIGNAL_JUMLAH
#define SIGNAL_JUMLAH               32
#endif

/* Signal standar - menggunakan dari konstanta.h */
/* SIGNAL_HUP, SIGNAL_INT, dll sudah didefinisikan */

/* Signal action */
#define SIGNAL_ACT_DEFAULT          0
#define SIGNAL_ACT_IGNORE           1
#define SIGNAL_ACT_HANDLER          2

/* Signal handler special values */
#define SIGNAL_HANDLER_DEFAULT      ((void (*)(int))0)
#define SIGNAL_HANDLER_IGNORE       ((void (*)(int))1)
#define SIGNAL_HANDLER_ERROR        ((void (*)(int))-1)

/*
 * ===========================================================================
 * CLONE FLAGS (untuk sys_clone)
 * ===========================================================================
 */

#define CLONE_VM                    0x00000100  /* Share memory space */
#define CLONE_FS                    0x00000200  /* Share fs info */
#define CLONE_FILES                 0x00000400  /* Share fd table */
#define CLONE_SIGHAND               0x00000800  /* Share signal handlers */
#define CLONE_PTRACE                0x00002000  /* Set ptrace flag */
#define CLONE_VFORK                 0x00004000  /* Parent blocked */
#define CLONE_PARENT                0x00008000  /* Same parent */
#define CLONE_THREAD                0x00010000  /* Same thread group */
#define CLONE_NEWNS                 0x00020000  /* New namespace */
#define CLONE_SYSVSEM               0x00040000  /* Share sysv sem */
#define CLONE_SETTLS                0x00080000  /* Set TLS */
#define CLONE_PARENT_SETTID         0x00100000  /* Set TID in parent */
#define CLONE_CHILD_CLEARTID        0x00200000  /* Clear TID in child */
#define CLONE_DETACHED              0x00400000  /* Detached */
#define CLONE_UNTRACED              0x00800000  /* Not traced */
#define CLONE_CHILD_SETTID          0x01000000  /* Set TID in child */

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
struct page_directory;

/*
 * ===========================================================================
 * KONSTANTA VMA (VIRTUAL MEMORY AREA FLAGS)
 * ===========================================================================
 */

/* Flag untuk VMA (Virtual Memory Area) */
#define VMA_FLAG_NONE           0x0000
#define VMA_FLAG_READ           0x0001  /* Dapat dibaca */
#define VMA_FLAG_WRITE          0x0002  /* Dapat ditulis */
#define VMA_FLAG_EXEC           0x0004  /* Dapat dieksekusi */
#define VMA_FLAG_STACK          0x0008  /* Region stack */
#define VMA_FLAG_HEAP           0x0010  /* Region heap */
#define VMA_FLAG_SHARED         0x0020  /* Memori shared */
#define VMA_FLAG_PRIVATE        0x0040  /* Memori private (COW) */
#define VMA_FLAG_ANONYMOUS      0x0080  /* Tidak mapped ke file */
#define VMA_FLAG_FIXED          0x0100  /* Alamat fixed */
#define VMA_FLAG_GROWSDOWN      0x0200  /* Stack grows down */
#define VMA_FLAG_GROWSUP        0x0400  /* Stack grows up */
#define VMA_FLAG_LOCKED         0x0800  /* Locked in memory */

/*
 * ===========================================================================
 * STRUKTUR VM DESCRIPTOR (VIRTUAL MEMORY DESCRIPTOR)
 * ===========================================================================
 */

#ifndef _VMA_TYPES_SUPPRESSED
/* Struktur VMA (Virtual Memory Area) */
#ifndef _VMA_T_DEFINED
#define _VMA_T_DEFINED
typedef struct vma {
    alamat_virtual_t mulai;     /* Alamat awal */
    alamat_virtual_t akhir;     /* Alamat akhir */
    tak_bertanda32_t flag;      /* Flag VMA */
    tak_bertanda32_t offset;    /* Offset dalam file */
    struct vma *next;           /* VMA berikutnya */
    struct vma *prev;           /* VMA sebelumnya */
    void *file;                 /* File yang di-map (jika ada) */
} vma_t;
#endif /* _VMA_T_DEFINED */

/* Struktur VM Descriptor */
#ifndef _VM_DESCRIPTOR_T_DEFINED
#define _VM_DESCRIPTOR_T_DEFINED
typedef struct vm_descriptor {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t ref_count;     /* Reference count */
    vma_t *vma_list;                /* List VMA */
    tak_bertanda64_t cr3;           /* Page directory base */
    tak_bertanda64_t total_size;    /* Total ukuran mapped */
    tak_bertanda32_t vma_count;     /* Jumlah VMA */
    tak_bertanda32_t page_count;    /* Jumlah halaman */
    struct vm_descriptor *next;     /* Untuk list */
    struct vm_descriptor *prev;     /* Untuk list */
} vm_descriptor_t;
#endif /* _VM_DESCRIPTOR_T_DEFINED */
#endif /* _VMA_TYPES_SUPPRESSED */

/*
 * ===========================================================================
 * STRUKTUR STATISTIK (STATISTICS STRUCTURES)
 * ===========================================================================
 */

/* Statistik memori proses */
typedef struct statistik_memori {
    ukuran_t total;         /* Total memori */
    ukuran_t kode;          /* Memori kode */
    ukuran_t data;          /* Memori data */
    ukuran_t stack;         /* Memori stack */
    ukuran_t heap;          /* Memori heap */
    ukuran_t shared;        /* Memori shared */
    ukuran_t libraries;     /* Memori libraries */
} statistik_memori_t;

/* Statistik CPU proses */
typedef struct statistik_cpu {
    tak_bertanda64_t user;      /* Waktu user mode */
    tak_bertanda64_t system;    /* Waktu system mode */
    tak_bertanda64_t idle;      /* Waktu idle */
    tak_bertanda64_t iowait;    /* Waktu menunggu I/O */
    tak_bertanda64_t irq;       /* Waktu IRQ handling */
    tak_bertanda64_t softirq;   /* Waktu softirq */
} statistik_cpu_t;

/* Statistik I/O proses */
typedef struct statistik_io {
    tak_bertanda64_t read_bytes;    /* Bytes dibaca */
    tak_bertanda64_t write_bytes;   /* Bytes ditulis */
    tak_bertanda64_t read_ops;      /* Operasi baca */
    tak_bertanda64_t write_ops;     /* Operasi tulis */
} statistik_io_t;

/*
 * ===========================================================================
 * STRUKTUR CONTEXT CPU (CPU CONTEXT STRUCTURE)
 * ===========================================================================
 * Struktur ini menyimpan state CPU untuk context switching.
 * Ukuran field berbeda untuk 32-bit dan 64-bit.
 */

#if defined(PIGURA_ARSITEKTUR_64BIT)

/*
 * Context 64-bit untuk x86_64 dan ARM64
 */
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
    tak_bertanda16_t _pad[3];
    
    /* Page directory base */
    tak_bertanda64_t cr3;       /* Page directory */
    
#if defined(ARSITEKTUR_ARM64)
    /* ARM64 specific registers */
    tak_bertanda64_t sp_el0;    /* Stack pointer EL0 */
    tak_bertanda64_t sp_el1;    /* Stack pointer EL1 */
    tak_bertanda64_t elr_el1;   /* Exception link register */
    tak_bertanda64_t spsr_el1;  /* Saved program status */
    tak_bertanda64_t tpidr_el0; /* Thread pointer */
    tak_bertanda64_t tpidrro_el0; /* Thread pointer read-only */
    tak_bertanda64_t fp;        /* Frame pointer (x29) */
    tak_bertanda64_t lr;        /* Link register (x30) */
#endif
} cpu_context_t;

#else /* PIGURA_ARSITEKTUR_32BIT */

/*
 * Context 32-bit untuk x86 dan ARM
 */
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
    tak_bertanda16_t _pad[1];
    
    /* Page directory base */
    tak_bertanda32_t cr3;       /* Page directory */
    
#if defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)
    /* ARM specific registers */
    tak_bertanda32_t sp_usr;    /* User mode SP */
    tak_bertanda32_t lr_usr;    /* User mode LR */
    tak_bertanda32_t spsr;      /* Saved program status */
    tak_bertanda32_t cpsr;      /* Current program status */
    tak_bertanda32_t sp_svc;    /* SVC mode SP */
    tak_bertanda32_t lr_svc;    /* SVC mode LR */
    tak_bertanda32_t sp_irq;    /* IRQ mode SP */
    tak_bertanda32_t lr_irq;    /* IRQ mode LR */
    tak_bertanda32_t spsr_irq;  /* SPSR for IRQ */
    tak_bertanda32_t sp_fiq;    /* FIQ mode SP */
    tak_bertanda32_t lr_fiq;    /* FIQ mode LR */
    tak_bertanda32_t spsr_fiq;  /* SPSR for FIQ */
    tak_bertanda32_t r8_fiq;    /* FIQ banked r8 */
    tak_bertanda32_t r9_fiq;    /* FIQ banked r9 */
    tak_bertanda32_t r10_fiq;   /* FIQ banked r10 */
    tak_bertanda32_t r11_fiq;   /* FIQ banked r11 */
    tak_bertanda32_t r12_fiq;   /* FIQ banked r12 */
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
    tanda32_t nice;                 /* Nice value */
    tak_bertanda32_t quantum;       /* Quantum remaining */
    
    /* Context CPU */
    struct cpu_context *context;    /* CPU context */
    
    /* Stack */
    void *stack_base;               /* Stack base address */
    void *stack_ptr;                /* Stack pointer (top) */
    ukuran_t stack_size;            /* Ukuran stack */
    
    /* Kernel stack untuk privilege switch */
    void *kstack_base;              /* Kernel stack base */
    void *kstack_ptr;               /* Kernel stack pointer */
    ukuran_t kstack_size;           /* Kernel stack size */
    
    /* Scheduling */
    tak_bertanda64_t cpu_time;       /* CPU time used */
    tak_bertanda64_t start_time;     /* Start time */
    tak_bertanda64_t wake_time;      /* Wake time for sleep */
    tak_bertanda32_t block_reason;   /* Alasan block */
    
    /* Exit status */
    tanda32_t exit_code;            /* Exit code */
    
    /* Linked list */
    struct thread *next;            /* Thread berikutnya */
    struct thread *prev;            /* Thread sebelumnya */
    
    /* Join/Wait */
    struct thread *waiting;         /* Thread yang menunggu */
    void *retval;                   /* Return value */
    tak_bertanda32_t join_count;     /* Jumlah thread join */
    
    /* Thread-local storage */
    void *tls;                      /* TLS pointer */
    tak_bertanda32_t tls_size;      /* TLS size */
    
    /* Signal */
    tak_bertanda32_t signal_mask;   /* Signal mask */
    tak_bertanda32_t signal_pending;/* Signal pending */
    
    /* Proses parent */
    struct proses *proses;          /* Pointer ke proses */
    
    /* CPU affinity */
    tak_bertanda32_t cpu_affinity;  /* CPU affinity mask */
    tak_bertanda32_t last_cpu;      /* CPU terakhir */
    
    /* Accounting */
    tak_bertanda64_t ctx_switches;  /* Context switch count */
} thread_t;

/*
 * ===========================================================================
 * STRUKTUR SIGNAL HANDLER
 * ===========================================================================
 */

typedef struct signal_handler {
    void (*handler)(int);           /* Handler function */
    tak_bertanda32_t flags;         /* SA_* flags */
    tak_bertanda32_t mask;          /* Signal mask during handler */
    void (*restorer)(void);         /* Signal restorer */
    tak_bertanda64_t call_count;    /* Jumlah pemanggilan */
} signal_handler_t;

/* Signal handler flags */
#define SA_NOCLDSTOP            0x00000001
#define SA_NOCLDWAIT            0x00000002
#define SA_SIGINFO              0x00000004
#define SA_ONSTACK              0x00000008
#define SA_RESTART              0x00000010
#define SA_NODEFER              0x00000020
#define SA_RESETHAND            0x00000040

/*
 * ===========================================================================
 * STRUKTUR SIGINFO (SIGNAL INFO STRUCTURE)
 * ===========================================================================
 * Struktur untuk menyimpan informasi signal yang dikirim ke proses.
 */

typedef struct siginfo {
    tak_bertanda32_t si_signo;      /* Nomor signal */
    tak_bertanda32_t si_code;       /* Kode sumber signal */
    tak_bertanda32_t si_errno;      /* Error number (jika ada) */
    pid_t si_pid;                   /* PID pengirim */
    uid_t si_uid;                   /* UID pengirim */
    void *si_addr;                  /* Alamat fault */
    tak_bertanda32_t si_status;     /* Exit status atau signal */
    tak_bertanda32_t si_band;       /* Band event */
    union {
        struct {
            pid_t _pid;             /* PID child */
            uid_t _uid;             /* UID child */
            tak_bertanda32_t _status;/* Status */
            tak_bertanda64_t _utime;/* User time */
            tak_bertanda64_t _stime;/* System time */
        } _sigchild;
        struct {
            void *_addr;            /* Fault address */
            tak_bertanda32_t _trapno;/* Trap number */
        } _sigfault;
        struct {
            tak_bertanda64_t _band; /* Band event */
            tak_bertanda32_t _fd;   /* File descriptor */
        } _sigpoll;
    } _sifields;
} siginfo_t;

#define si_pid_child    _sifields._sigchild._pid
#define si_uid_child    _sifields._sigchild._uid
#define si_status_child _sifields._sigchild._status
#define si_utime_child  _sifields._sigchild._utime
#define si_stime_child  _sifields._sigchild._stime
#define si_addr_fault   _sifields._sigfault._addr
#define si_trapno_fault _sifields._sigfault._trapno

/*
 * ===========================================================================
 * STRUKTUR FILE OPERATIONS (FILE OPERATIONS STRUCTURE)
 * ===========================================================================
 */

/* Forward declaration */
struct file_descriptor;

/* Struktur file operations */
typedef struct file_operations {
    status_t (*open)(struct file_descriptor *fd, const char *path, 
                     tak_bertanda32_t flags);
    status_t (*close)(struct file_descriptor *fd);
    tak_bertanda64_t (*read)(struct file_descriptor *fd, void *buf, 
                             ukuran_t count);
    tak_bertanda64_t (*write)(struct file_descriptor *fd, const void *buf, 
                              ukuran_t count);
    tak_bertanda64_t (*lseek)(struct file_descriptor *fd, tak_bertanda64_t offset,
                              tak_bertanda32_t whence);
    status_t (*ioctl)(struct file_descriptor *fd, tak_bertanda32_t cmd, 
                      void *arg);
    status_t (*mmap)(struct file_descriptor *fd, void *addr, ukuran_t len,
                     tak_bertanda32_t prot, tak_bertanda32_t flags,
                     tak_bertanda64_t offset);
    status_t (*munmap)(struct file_descriptor *fd, void *addr, ukuran_t len);
    struct file_descriptor *(*dup)(struct file_descriptor *fd);
    status_t (*stat)(struct file_descriptor *fd, void *statbuf);
    status_t (*truncate)(struct file_descriptor *fd, tak_bertanda64_t length);
} file_operations_t;

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
    tak_bertanda32_t mode;          /* Mode file */
    struct file_operations *ops;    /* File operations */
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
    uid_t suid;                     /* Saved UID */
    gid_t sgid;                     /* Saved GID */
    uid_t fsuid;                    /* Filesystem UID */
    gid_t fsgid;                    /* Filesystem GID */
    pid_t pgid;                     /* Process group ID */
    pid_t sid;                      /* Session ID */
    
    /* Groups */
    gid_t groups[32];               /* Supplementary groups */
    tak_bertanda32_t ngroups;       /* Number of groups */
    
    /* Nama */
    char nama[32];                  /* Nama proses */
    char cwd[256];                  /* Current working directory */
    char root[256];                 /* Root directory */
    char exe[256];                  /* Executable path */
    
    /* Status dan flag */
    tak_bertanda32_t status;        /* Status proses */
    tak_bertanda32_t flags;         /* Flag proses */
    tak_bertanda32_t prioritas;     /* Prioritas */
    tanda32_t nice;                 /* Nice value */
    tanda32_t exit_code;            /* Exit code */
    tak_bertanda32_t exit_signal;   /* Signal yang menyebabkan exit */
    
    /* Memory management */
    struct vm_descriptor *vm;       /* Virtual memory descriptor */
    struct page_directory *pgdir;   /* Page directory */
    alamat_virtual_t entry_point;   /* Entry point */
    tak_bertanda64_t brk_start;     /* Start of heap */
    tak_bertanda64_t brk_current;   /* Current brk */
    tak_bertanda64_t brk_end;       /* End of heap */
    tak_bertanda64_t start_code;    /* Start of code segment */
    tak_bertanda64_t end_code;      /* End of code segment */
    tak_bertanda64_t start_data;    /* Start of data segment */
    tak_bertanda64_t end_data;      /* End of data segment */
    tak_bertanda64_t start_stack;   /* Start of stack */
    
    /* Thread management */
    thread_t *threads;              /* List thread */
    thread_t *main_thread;          /* Main thread */
    tak_bertanda32_t thread_count;  /* Jumlah thread */
    tak_bertanda32_t thread_limit;  /* Batas thread */
    
    /* File descriptors */
    struct file_descriptor *fd_table[FD_MAKS_PER_PROSES];
    tak_bertanda32_t fd_count;      /* Jumlah FD aktif */
    tak_bertanda32_t fd_next_free;  /* FD free slot hint */
    tak_bertanda32_t fd_max;        /* Max FD untuk proses ini */
    
    /* Signal handling */
    tak_bertanda32_t signal_mask;   /* Signal mask */
    tak_bertanda32_t signal_pending;/* Signal pending */
    tak_bertanda32_t signal_ignored;/* Signal ignored */
    signal_handler_t signal_handlers[SIGNAL_JUMLAH];
    void *signal_trampoline;        /* Signal trampoline */
    tak_bertanda32_t saved_signal_mask;
    tak_bertanda32_t signal_count;  /* Total signals received */
    
    /* Scheduling */
    tak_bertanda64_t cpu_time;      /* CPU time used */
    tak_bertanda64_t start_time;    /* Start time */
    tak_bertanda64_t end_time;      /* End time */
    tak_bertanda64_t real_start;    /* Real start time */
    tak_bertanda32_t quantum;       /* Quantum remaining */
    tak_bertanda32_t rt_priority;   /* Realtime priority */
    
    /* CPU affinity */
    tak_bertanda32_t cpu_affinity;  /* CPU affinity mask */
    tak_bertanda32_t last_cpu;      /* CPU terakhir */
    
    /* Linked list */
    struct proses *next;            /* Proses berikutnya */
    struct proses *prev;            /* Proses sebelumnya */
    
    /* Child processes */
    struct proses *children;        /* List child process */
    struct proses *sibling;         /* Sibling process */
    struct proses *parent;          /* Parent process */
    tak_bertanda32_t child_count;   /* Jumlah child */
    
    /* Wait state */
    tak_bertanda32_t wait_state;    /* State untuk wait */
    pid_t wait_pid;                 /* PID yang ditunggu */
    tak_bertanda32_t wait_options;  /* Option wait */
    tak_bertanda32_t zombie_children;/* Jumlah child zombie */
    
    /* Zombie list */
    struct proses *next_zombie;     /* Link ke zombie berikutnya */
    bool_t wait_collected;          /* Sudah di-collect oleh wait */
    
    /* Exit timing */
    tak_bertanda64_t exit_time;     /* Waktu exit */
    
    /* vfork state */
    pid_t vfork_child;              /* vfork child PID */
    
    /* Clone/Thread state */
    pid_t *set_child_tid;           /* Alamat untuk set child TID */
    pid_t *clear_child_tid;         /* Alamat untuk clear child TID */
    
    /* Capabilities */
    tak_bertanda64_t cap_effective;  /* Effective capabilities */
    tak_bertanda64_t cap_permitted;  /* Permitted capabilities */
    tak_bertanda64_t cap_inheritable;/* Inheritable capabilities */
    
    /* Resource limits */
    ukuran_t limit_stack;           /* Stack size limit */
    ukuran_t limit_data;            /* Data size limit */
    ukuran_t limit_core;            /* Core dump limit */
    ukuran_t limit_memory;          /* Memory limit */
    tak_bertanda64_t limit_cpu;     /* CPU time limit */
    tak_bertanda32_t limit_nofile;  /* Open files limit */
    tak_bertanda32_t limit_nproc;   /* Process limit */
    
    /* Statistik */
    statistik_memori_t mem_stat;    /* Statistik memori */
    statistik_cpu_t cpu_stat;       /* Statistik CPU */
    statistik_io_t io_stat;         /* Statistik I/O */
    
    /* Security */
    tak_bertanda32_t securebits;    /* Secure computing */
    void *seccomp_filter;           /* Seccomp filter */
    
    /* Namespace */
    void *ns_pid;                   /* PID namespace */
    void *ns_mnt;                   /* Mount namespace */
    void *ns_net;                   /* Network namespace */
} proses_t;

/*
 * ===========================================================================
 * STRUKTUR RUNQUEUE (RUNQUEUE STRUCTURE)
 * ===========================================================================
 */

typedef struct runqueue {
    thread_t *head;                 /* Thread pertama */
    thread_t *tail;                 /* Thread terakhir */
    tak_bertanda32_t count;         /* Jumlah thread */
    tak_bertanda64_t total_time;    /* Total CPU time */
} runqueue_t;

/*
 * ===========================================================================
 * STRUKTUR SCHEDULER (SCHEDULER STRUCTURE)
 * ===========================================================================
 */

#define SCHEDULER_ANTRIAN_JUMLAH    8   /* Jumlah antrian prioritas */

typedef struct scheduler {
    /* Antrian per prioritas */
    runqueue_t antrian[SCHEDULER_ANTRIAN_JUMLAH];
    
    /* Runqueue untuk realtime */
    runqueue_t rt_queue;            /* Realtime queue */
    runqueue_t dl_queue;            /* Deadline queue */
    
    /* Statistik */
    tak_bertanda32_t total_proses;      /* Total proses runnable */
    tak_bertanda32_t total_threads;     /* Total thread runnable */
    tak_bertanda32_t tick_count;        /* Counter tick */
    tak_bertanda64_t context_switches;  /* Jumlah context switch */
    
    /* Boost untuk anti-starvation */
    tak_bertanda32_t last_boost;        /* Tick boost terakhir */
    tak_bertanda32_t boost_interval;    /* Interval boost */
    
    /* Load average */
    tak_bertanda32_t load_avg[3];       /* 1, 5, 15 min average */
    
    /* Lock */
    spinlock_t lock;                    /* Lock untuk thread safety */
    
    /* Flags */
    tak_bertanda32_t flags;             /* Flag scheduler */
    
    /* Idle thread */
    thread_t *idle_thread;              /* Pointer ke idle thread */
    
    /* Current running */
    thread_t *current_thread;           /* Thread yang sedang jalan */
    
    /* Preempt count */
    tak_bertanda32_t preempt_count;     /* Preemption counter */
    
    /* CPU info */
    tak_bertanda32_t cpu_id;            /* CPU ID */
    tak_bertanda64_t idle_time;         /* Total idle time */
    tak_bertanda64_t last_tick;         /* Last tick timestamp */
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

/* Alias untuk kompatibilitas */
#define proses_create           proses_buat
#define proses_get_current      proses_dapat_saat_ini
#define proses_set_current      proses_set_saat_ini
#define proses_get_kernel       proses_dapat_kernel
#define proses_get_idle         proses_dapat_idle
#define proses_get_count        proses_dapat_jumlah
#define proses_get_parent       proses_dapat_parent

/* Cari dan dapatkan proses */
proses_t *proses_cari(pid_t pid);
proses_t *proses_cari_by_name(const char *nama);
proses_t *proses_dapat_saat_ini(void);
proses_t *proses_dapat_kernel(void);
proses_t *proses_dapat_idle(void);
proses_t *proses_dapat_parent(proses_t *child);

/* Set proses */
void proses_set_saat_ini(proses_t *proses);

/* Jumlah proses */
tak_bertanda32_t proses_dapat_jumlah(void);
tak_bertanda32_t proses_dapat_jumlah_zombie(void);
tak_bertanda32_t proses_dapat_jumlah_total(void);

/* Exit proses */
status_t proses_exit(pid_t pid, tanda32_t exit_code);

/* Kill proses */
status_t proses_kill(pid_t pid, tak_bertanda32_t signal);

/* Prioritas */
status_t proses_set_prioritas(pid_t pid, tak_bertanda32_t prioritas);
tak_bertanda32_t proses_dapat_prioritas(pid_t pid);
status_t proses_set_nice(pid_t pid, tanda32_t nice);
tanda32_t proses_dapat_nice(pid_t pid);

/* Child process */
void proses_tambah_child(proses_t *parent, proses_t *child);
void proses_hapus_child(proses_t *parent, proses_t *child);
proses_t *proses_dapat_child(proses_t *parent, pid_t pid);

/* List management */
void proses_tambah_ke_list(proses_t *proses);
void proses_hapus_dari_list(proses_t *proses);

/* Validasi */
bool_t proses_valid(proses_t *proses);
bool_t proses_pid_valid(pid_t pid);

/* Statistik */
void proses_dapat_statistik(tak_bertanda64_t *created,
                            tak_bertanda64_t *exited,
                            tak_bertanda64_t *switches);

/* Print */
void proses_print_info(pid_t pid);
void proses_print_list(void);
void proses_print_tree(void);

/* Bebaskan struktur proses */
void proses_bebaskan(proses_t *proses);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI THREAD (THREAD FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t thread_init(void);

/* Buat dan hapus thread */
tid_t thread_buat(pid_t pid, void (*entry)(void *), void *arg,
                  tak_bertanda32_t flags);
void thread_exit(void *retval);
status_t thread_join(tid_t tid, void **retval);
status_t thread_detach(tid_t tid);

/* Thread management */
void thread_yield(void);
tid_t thread_self(void);
status_t thread_cancel(tid_t tid);

/* Prioritas thread */
status_t thread_set_prioritas(tid_t tid, tak_bertanda32_t prioritas);
tak_bertanda32_t thread_dapat_prioritas(tid_t tid);

/* Alokasi thread */
thread_t *thread_alokasi(void);
void thread_bebaskan(thread_t *thread);

/* Stack */
void *thread_alokasi_stack(ukuran_t size, bool_t is_user);
void thread_bebaskan_stack(void *stack, ukuran_t size, bool_t is_user);

/* Kernel stack */
void *thread_alokasi_kstack(void);
void thread_bebaskan_kstack(void *stack);

/* Cari thread */
thread_t *thread_cari(tid_t tid);
thread_t *thread_cari_di_proses(pid_t pid, tid_t tid);

/* Status */
bool_t thread_valid(thread_t *thread);
void thread_set_status(thread_t *thread, tak_bertanda32_t status);

/* Statistik */
void thread_dapat_statistik(tak_bertanda64_t *created,
                            tak_bertanda64_t *exited,
                            tak_bertanda64_t *active);

/* Print */
void thread_print_info(tid_t tid);
void thread_print_list(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI SCHEDULER (SCHEDULER FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t scheduler_init(void);
status_t scheduler_init_cpu(tak_bertanda32_t cpu_id);

/* Tambah dan hapus */
status_t scheduler_tambah_thread(thread_t *thread);
status_t scheduler_hapus_thread(thread_t *thread);

/* Schedule */
void scheduler_tick(void);
void scheduler_schedule(void);
status_t scheduler_yield(void);
void scheduler_preempt(void);

/* Block dan unblock */
status_t scheduler_block(tak_bertanda32_t reason);
status_t scheduler_block_timeout(tak_bertanda32_t reason,
                                 tak_bertanda64_t timeout);
status_t scheduler_unblock(thread_t *thread);

/* Prioritas */
status_t scheduler_set_prioritas(thread_t *thread,
                                 tak_bertanda32_t prioritas);
status_t scheduler_boost_prioritas(thread_t *thread);

/* Load */
tak_bertanda32_t scheduler_dapat_beban(void);
void scheduler_dapat_load_avg(tak_bertanda32_t *avg1,
                              tak_bertanda32_t *avg5,
                              tak_bertanda32_t *avg15);

/* Check */
bool_t scheduler_perlu_resched(void);
void scheduler_set_need_resched(void);
void scheduler_clear_need_resched(void);

/* Current */
thread_t *scheduler_dapat_current(void);
void scheduler_set_current(thread_t *thread);

/* CPU affinity */
status_t scheduler_set_affinity(thread_t *thread,
                                tak_bertanda32_t mask);
tak_bertanda32_t scheduler_dapat_affinity(thread_t *thread);

/* Statistik */
void scheduler_dapat_statistik(tak_bertanda64_t *switches,
                               tak_bertanda64_t *preemptions,
                               tak_bertanda64_t *yields,
                               tak_bertanda64_t *timeouts);

/* Print */
void scheduler_print_stats(void);
void scheduler_print_queues(void);
void scheduler_print_load(void);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI CONTEXT (CONTEXT FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi */
status_t context_init(void);

/* Buat dan hapus context */
cpu_context_t *context_buat(alamat_virtual_t entry_point, void *stack_ptr,
                            bool_t is_user);
void context_hancurkan(cpu_context_t *ctx);

/* Context switch */
void context_switch(proses_t *old_proses, proses_t *new_proses);
void context_switch_thread(thread_t *old_thread, thread_t *new_thread);
void context_switch_to(cpu_context_t *ctx);

/* Set context */
status_t context_set_entry(cpu_context_t *ctx, alamat_virtual_t entry);
status_t context_set_stack(cpu_context_t *ctx, void *stack_ptr);
status_t context_set_cr3(cpu_context_t *ctx, tak_bertanda64_t cr3);
void context_set_return(cpu_context_t *ctx, tanda64_t retval);

/* Get context */
void *context_dapat_stack(cpu_context_t *ctx);
alamat_virtual_t context_dapat_entry(cpu_context_t *ctx);
tak_bertanda64_t context_dapat_cr3(cpu_context_t *ctx);

/* Setup context */
status_t context_setup_user(cpu_context_t *ctx, alamat_virtual_t entry,
                            alamat_virtual_t stack_top,
                            tak_bertanda32_t argc, tak_bertanda32_t argv);
status_t context_setup_kernel(cpu_context_t *ctx,
                              alamat_virtual_t entry, void *arg);

/* Duplikasi context */
cpu_context_t *context_dup(cpu_context_t *ctx);

/* Return ke user */
void context_return_to_user(cpu_context_t *ctx);

/* Validasi */
bool_t context_valid(cpu_context_t *ctx);

/* Statistik */
void context_dapat_statistik(tak_bertanda64_t *total,
                             tak_bertanda64_t *k_to_u,
                             tak_bertanda64_t *u_to_k,
                             tak_bertanda64_t *u_to_u);

/* Print */
void context_print_info(cpu_context_t *ctx);

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
tanda32_t sys_execvpe(const char *file, char *const argv[],
                      char *const envp[]);

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

/* Inisialisasi */
status_t signal_init(void);

/* Kirim signal */
status_t signal_kirim(pid_t pid, tak_bertanda32_t sig);
status_t signal_kirim_ke_proses(proses_t *target, tak_bertanda32_t sig);
status_t signal_kirim_ke_group(pid_t pgid, tak_bertanda32_t sig);

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

/* Info */
bool_t elf_validasi(const void *header);
tak_bertanda32_t elf_dapat_class(const void *header);
tak_bertanda32_t elf_dapat_machine(const void *header);

/* Statistik */
void elf_dapat_statistik(tak_bertanda64_t *files,
                         tak_bertanda64_t *segments,
                         tak_bertanda64_t *bytes,
                         tak_bertanda64_t *errors);

/* Print */
void elf_print_stats(void);
void elf_print_info(void *buffer);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI TSS (TSS FUNCTIONS) - x86/x86_64 ONLY
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

/* Copy dengan validasi */
status_t copy_from_user(void *dest, const void *src, ukuran_t size);
status_t copy_to_user(void *dest, const void *src, ukuran_t size);
status_t copy_string_from_user(char *dest, const char *src,
                               ukuran_t max_len);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI HELPER (HELPER FUNCTIONS)
 * ===========================================================================
 */

/* Get quantum berdasarkan prioritas */
tak_bertanda32_t dapatkan_quantum(tak_bertanda32_t prioritas);

/* Dapatkan index runqueue */
tak_bertanda32_t dapatkan_runqueue_idx(tak_bertanda32_t prioritas);

/* Cek apakah proses adalah kernel process */
bool_t proses_adalah_kernel(proses_t *proses);

/* Dapatkan parent yang bisa menerima signal */
proses_t *proses_dapat_parent_alive(proses_t *child);

/* Cek apakah proses memiliki children */
bool_t proses_punya_children(proses_t *proses);

/* Cek apakah proses memiliki zombie children */
bool_t proses_punya_zombie(proses_t *proses);

/* Cek apakah proses dalam status runnable */
bool_t proses_runnable(proses_t *proses);

/* Cek apakah thread dalam status runnable */
bool_t thread_runnable(thread_t *thread);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI VM (VIRTUAL MEMORY FUNCTIONS)
 * ===========================================================================
 */

/* Inisialisasi VM */
status_t vm_init(void);

#ifndef _VMA_TYPES_SUPPRESSED
/* Buat dan hancurkan address space */
vm_descriptor_t *vm_create_address_space(void);
void vm_destroy_address_space(vm_descriptor_t *vm);

/* Operasi VM */
alamat_virtual_t vm_alloc(vm_descriptor_t *vm, ukuran_t size, 
                          tak_bertanda32_t flags, alamat_virtual_t hint);
void vm_free(vm_descriptor_t *vm, alamat_virtual_t addr, ukuran_t size);
status_t vm_copy_regions(vm_descriptor_t *src, vm_descriptor_t *dst);

/* Query VM region */
bool_t vm_query(vm_descriptor_t *vm, alamat_virtual_t addr,
                alamat_fisik_t *phys, tak_bertanda32_t *flags, void **vma);

/* Validasi */
bool_t vm_validate_address(vm_descriptor_t *vm, alamat_virtual_t addr);
#endif /* _VMA_TYPES_SUPPRESSED */

/* Get runqueue index */
tak_bertanda32_t dapatkan_runqueue_idx(tak_bertanda32_t prioritas);

/* Validasi user pointer */
bool_t validasi_pointer_user(const void *ptr);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI BERKAS (FILE FUNCTIONS)
 * ===========================================================================
 */

/* Tutup file descriptor */
void berkas_tutup(struct file_descriptor *fd);

/* Dapatkan file descriptor */
struct file_descriptor *berkas_dapat(proses_t *proses, tak_bertanda32_t fd);

/* Alokasi fd baru */
tak_bertanda32_t berkas_alokasi_fd(proses_t *proses);

/*
 * ===========================================================================
 * TIPE ALIAS UNTUK VFS (VFS TYPE ALIASES)
 * ===========================================================================
 */

/* Alias berkas_t untuk file_descriptor_t */
typedef struct file_descriptor berkas_t;

/*
 * ===========================================================================
 * KONSTANTA BERKAS (FILE CONSTANTS)
 * ===========================================================================
 */

/* Flag untuk berkas_buka */
#ifndef BERKAS_BACA
#define BERKAS_BACA         0x0001  /* Buka untuk baca */
#endif
#ifndef BERKAS_TULIS
#define BERKAS_TULIS        0x0002  /* Buka untuk tulis */
#endif
#ifndef BERKAS_BUAT
#define BERKAS_BUAT         0x0100  /* Buat file baru */
#endif
#ifndef BERKAS_TRUNCATE
#define BERKAS_TRUNCATE     0x0200  /* Truncate file */
#endif
#ifndef BERKAS_APPEND
#define BERKAS_APPEND       0x0400  /* Append mode */
#endif

/* Seek constants */
#ifndef BERKAS_SEEK_SET
#define BERKAS_SEEK_SET     0       /* Dari awal file */
#endif
#ifndef BERKAS_SEEK_CUR
#define BERKAS_SEEK_CUR     1       /* Dari posisi saat ini */
#endif
#ifndef BERKAS_SEEK_END
#define BERKAS_SEEK_END     2       /* Dari akhir file */
#endif

/*
 * ===========================================================================
 * STRUKTUR BINARY INFO (BINARY INFO STRUCTURE)
 * ===========================================================================
 * Informasi tentang berkas executable yang dimuat.
 */

typedef struct binary_info {
    alamat_virtual_t entry;     /* Entry point */
    alamat_virtual_t phdr;      /* Program header address */
    ukuran_t phnum;             /* Number of program headers */
    ukuran_t phent;             /* Program header entry size */
    tak_bertanda32_t type;      /* Type: 32 atau 64 */
    tak_bertanda32_t flags;     /* Flags tambahan */
} binary_info_t;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI VFS TAMBAHAN (ADDITIONAL VFS FUNCTIONS)
 * ===========================================================================
 */

/* Buka berkas */
berkas_t *berkas_buka(const char *path, tak_bertanda32_t flags);

/* Baca dari berkas */
ukuran_t berkas_baca(berkas_t *berkas, void *buf, ukuran_t count);

/* Tulis ke berkas */
ukuran_t berkas_tulis(berkas_t *berkas, const void *buf, ukuran_t count);

/* Seek dalam berkas */
tak_bertanda64_t berkas_seek(berkas_t *berkas, tak_bertanda64_t offset,
                            tak_bertanda32_t whence);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI VM (VIRTUAL MEMORY FUNCTIONS)
 * ===========================================================================
 */

#ifndef _VMA_TYPES_SUPPRESSED
/* Alokasi memori di alamat tertentu */
status_t vm_alloc_at(vm_descriptor_t *vm, ukuran_t size, tak_bertanda32_t flags,
                     alamat_virtual_t addr);
#endif /* _VMA_TYPES_SUPPRESSED */

#endif /* PROSES_PROSES_H */
