/*
 * PIGURA OS - PROSES x86
 * ----------------------
 * Implementasi manajemen proses untuk arsitektur x86.
 *
 * Berkas ini berisi implementasi fungsi-fungsi untuk manajemen
 * proses pada arsitektur x86, termasuk context switching,
 * setup TSS, dan operasi ring switch.
 *
 * Arsitektur: x86 (32-bit)
 * Versi: 1.0
 */

#include "../../inti/kernel.h"

/*
 * ============================================================================
 * KONSTANTA PROSES
 * ============================================================================
 */

/* Jumlah maksimum proses */
#define MAKS_PROSES_SISTEM      64

/* Ukuran stack sudah didefinisikan di konstanta.h, gunakan nilai dari sana */
/* #define UKURAN_STACK_KERNEL  - gunakan dari konstanta.h */
/* #define UKURAN_STACK_USER    - gunakan dari konstanta.h */

/* Alamat kernel */
#define KERNEL_VIRTUAL_BASE     0xC0000000

/* Flag segment */
#define FLAG_SEGMENT_KERNEL     0x00
#define FLAG_SEGMENT_USER       0x03

/*
 * ============================================================================
 * STRUKTUR DATA
 * ============================================================================
 */

/* Context register untuk context switching */
struct context_cpu {
    tak_bertanda32_t edi;           /* Register edi */
    tak_bertanda32_t esi;           /* Register esi */
    tak_bertanda32_t ebx;           /* Register ebx */
    tak_bertanda32_t ebp;           /* Register ebp */
    tak_bertanda32_t eip;           /* Instruction pointer */
    tak_bertanda32_t esp;           /* Stack pointer */
    tak_bertanda32_t eflags;        /* Flags register */
    tak_bertanda32_t cr3;           /* Page directory */
    tak_bertanda16_t cs;            /* Code segment */
    tak_bertanda16_t ds;            /* Data segment */
    tak_bertanda16_t es;            /* Extra segment */
    tak_bertanda16_t fs;            /* F segment */
    tak_bertanda16_t gs;            /* G segment */
    tak_bertanda16_t ss;            /* Stack segment */
} __attribute__((packed));

/* Task State Segment (TSS) */
struct tss {
    tak_bertanda16_t link;          /* Link ke TSS sebelumnya */
    tak_bertanda16_t link_h;
    tak_bertanda32_t esp0;          /* Stack pointer ring 0 */
    tak_bertanda16_t ss0;           /* Stack segment ring 0 */
    tak_bertanda16_t ss0_h;
    tak_bertanda32_t esp1;          /* Stack pointer ring 1 */
    tak_bertanda16_t ss1;           /* Stack segment ring 1 */
    tak_bertanda16_t ss1_h;
    tak_bertanda32_t esp2;          /* Stack pointer ring 2 */
    tak_bertanda16_t ss2;           /* Stack segment ring 2 */
    tak_bertanda16_t ss2_h;
    tak_bertanda32_t cr3;           /* Page directory */
    tak_bertanda32_t eip;           /* Instruction pointer */
    tak_bertanda32_t eflags;        /* Flags */
    tak_bertanda32_t eax;           /* Register eax */
    tak_bertanda32_t ecx;           /* Register ecx */
    tak_bertanda32_t edx;           /* Register edx */
    tak_bertanda32_t ebx;           /* Register ebx */
    tak_bertanda32_t esp;           /* Stack pointer */
    tak_bertanda32_t ebp;           /* Base pointer */
    tak_bertanda32_t esi;           /* Register esi */
    tak_bertanda32_t edi;           /* Register edi */
    tak_bertanda16_t es;            /* Extra segment */
    tak_bertanda16_t es_h;
    tak_bertanda16_t cs;            /* Code segment */
    tak_bertanda16_t cs_h;
    tak_bertanda16_t ss;            /* Stack segment */
    tak_bertanda16_t ss_h;
    tak_bertanda16_t ds;            /* Data segment */
    tak_bertanda16_t ds_h;
    tak_bertanda16_t fs;            /* F segment */
    tak_bertanda16_t fs_h;
    tak_bertanda16_t gs;            /* G segment */
    tak_bertanda16_t gs_h;
    tak_bertanda16_t ldt;           /* Local descriptor table */
    tak_bertanda16_t ldt_h;
    tak_bertanda16_t trap;          /* Trap flag */
    tak_bertanda16_t iomap_base;    /* I/O map base */
} __attribute__((packed));

/* Struktur proses internal */
struct proses_internal {
    pid_t pid;                      /* Process ID */
    pid_t ppid;                     /* Parent process ID */
    proses_status_t status;         /* Status proses */
    prioritas_t prioritas;          /* Prioritas */
    char nama[MAKS_NAMA_PROSES];    /* Nama proses */
    struct context_cpu context;     /* Context CPU */
    tak_bertanda32_t *pd;           /* Page directory */
    tak_bertanda32_t kernel_stack;  /* Stack kernel */
    tak_bertanda32_t user_stack;    /* Stack user */
    bool_t sedang_jalan;            /* Flag sedang berjalan */
    tak_bertanda64_t waktu_mulai;   /* Waktu mulai */
    tak_bertanda64_t waktu_cpu;     /* Waktu CPU terpakai */
    struct proses_internal *parent; /* Pointer ke parent */
    struct proses_internal *next;   /* Pointer ke proses berikutnya */
};

/*
 * ============================================================================
 * VARIABEL GLOBAL
 * ============================================================================
 */

/* TSS kernel */
static struct tss g_tss_kernel __attribute__((aligned(16)));

/* Tabel proses */
static struct proses_internal g_tabel_proses[MAKS_PROSES_SISTEM];

/* Proses saat ini (internal, berbeda dengan g_proses_sekarang di kernel.h) */
static struct proses_internal *g_proses_sekarang_internal = NULL;

/* Proses kernel */
static struct proses_internal g_proses_kernel;

/* Stack untuk context switching */
static tak_bertanda8_t g_stack_context[UKURAN_STACK_KERNEL]
    __attribute__((aligned(16)));

/* Jumlah proses aktif */
static tak_bertanda32_t g_jumlah_proses = 0;

/* PID berikutnya */
static pid_t g_pid_berikutnya = 1;

/* Flag inisialisasi */
static bool_t g_proses_diinisialisasi = SALAH;

/*
 * ============================================================================
 * DEKLARASI FUNGSI EKSTERNAL
 * ============================================================================
 */

/* Entry point assembly untuk context switch */
extern void _context_switch(struct context_cpu *lama,
                            struct context_cpu *baru);

/* Entry point untuk user mode */
extern void _jump_to_usermode(tak_bertanda32_t entry,
                              tak_bertanda32_t stack);

/*
 * ============================================================================
 * FUNGSI INTERNAL - TSS
 * ============================================================================
 */

/*
 * _tss_init
 * ---------
 * Menginisialisasi TSS kernel.
 */
static void _tss_init(void)
{
    kernel_memset(&g_tss_kernel, 0, sizeof(struct tss));

    g_tss_kernel.ss0 = SELECTOR_KERNEL_DATA;
    g_tss_kernel.esp0 = (tak_bertanda32_t)(uintptr_t)&g_stack_context +
                        sizeof(g_stack_context);
    g_tss_kernel.iomap_base = sizeof(struct tss);
}

/*
 * _tss_load
 * ---------
 * Memuat TSS ke TR (Task Register).
 */
static void _tss_load(void)
{
    tak_bertanda16_t tr = SELECTOR_TSS;

    __asm__ __volatile__(
        "ltr %0\n\t"
        :
        : "r"(tr)
    );
}

/*
 * _tss_set_kernel_stack
 * ---------------------
 * Mengatur stack kernel di TSS.
 *
 * Parameter:
 *   esp0 - Alamat top stack kernel
 */
static void _tss_set_kernel_stack(tak_bertanda32_t esp0)
{
    g_tss_kernel.esp0 = esp0;
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - ALOKASI PROSES
 * ============================================================================
 */

/*
 * _alokasi_proses_slot
 * --------------------
 * Mencari slot kosong di tabel proses.
 *
 * Return:
 *   Index slot, atau -1 jika penuh
 */
static tanda32_t _alokasi_proses_slot(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < MAKS_PROSES_SISTEM; i++) {
        if (g_tabel_proses[i].status == PROSES_STATUS_BELUM ||
            g_tabel_proses[i].pid == 0) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * _alokasi_pid
 * ------------
 * Mengalokasikan PID baru.
 *
 * Return:
 *   PID baru
 */
static pid_t _alokasi_pid(void)
{
    pid_t pid = g_pid_berikutnya++;

    /* Wrap around jika perlu */
    if (g_pid_berikutnya >= (pid_t)MAKS_PROSES_SISTEM) {
        g_pid_berikutnya = 1;
    }

    return pid;
}

/*
 * _dealokasi_proses
 * -----------------
 * Membersihkan slot proses.
 *
 * Parameter:
 *   proses - Pointer ke struktur proses
 */
static void _dealokasi_proses(struct proses_internal *proses)
{
    if (proses == NULL) {
        return;
    }

    kernel_memset(proses, 0, sizeof(struct proses_internal));
    proses->status = PROSES_STATUS_BELUM;
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - CONTEXT
 * ============================================================================
 */

/*
 * _context_init_kernel
 * --------------------
 * Menginisialisasi context untuk proses kernel.
 *
 * Parameter:
 *   proses  - Pointer ke struktur proses
 *   entry   - Entry point
 *   stack   - Alamat stack
 */
static void _context_init_kernel(struct proses_internal *proses,
                                  tak_bertanda32_t entry,
                                  tak_bertanda32_t stack)
{
    if (proses == NULL) {
        return;
    }

    kernel_memset(&proses->context, 0, sizeof(struct context_cpu));

    proses->context.eip = entry;
    proses->context.esp = stack;
    proses->context.ebp = stack;
    proses->context.eflags = 0x202; /* IF = 1 */
    proses->context.cs = SELECTOR_KERNEL_CODE;
    proses->context.ds = SELECTOR_KERNEL_DATA;
    proses->context.es = SELECTOR_KERNEL_DATA;
    proses->context.fs = SELECTOR_KERNEL_DATA;
    proses->context.gs = SELECTOR_KERNEL_DATA;
    proses->context.ss = SELECTOR_KERNEL_DATA;
    proses->context.cr3 = cpu_read_cr3();
}

/*
 * _context_init_user
 * ------------------
 * Menginisialisasi context untuk proses user.
 *
 * Parameter:
 *   proses  - Pointer ke struktur proses
 *   entry   - Entry point
 *   stack   - Alamat stack user
 *   pd      - Page directory
 */
static void _context_init_user(struct proses_internal *proses,
                                tak_bertanda32_t entry,
                                tak_bertanda32_t stack,
                                tak_bertanda32_t pd)
{
    if (proses == NULL) {
        return;
    }

    kernel_memset(&proses->context, 0, sizeof(struct context_cpu));

    proses->context.eip = entry;
    proses->context.esp = stack;
    proses->context.ebp = stack;
    proses->context.eflags = 0x202; /* IF = 1 */
    proses->context.cs = SELECTOR_USER_CODE | FLAG_SEGMENT_USER;
    proses->context.ds = SELECTOR_USER_DATA | FLAG_SEGMENT_USER;
    proses->context.es = SELECTOR_USER_DATA | FLAG_SEGMENT_USER;
    proses->context.fs = SELECTOR_USER_DATA | FLAG_SEGMENT_USER;
    proses->context.gs = SELECTOR_USER_DATA | FLAG_SEGMENT_USER;
    proses->context.ss = SELECTOR_USER_DATA | FLAG_SEGMENT_USER;
    proses->context.cr3 = pd;
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - PAGE DIRECTORY
 * ============================================================================
 */

/*
 * _buat_page_directory
 * --------------------
 * Membuat page directory baru untuk proses.
 *
 * Return:
 *   Alamat page directory, atau 0 jika gagal
 */
static tak_bertanda32_t _buat_page_directory(void)
{
    tak_bertanda32_t *pd;
    tak_bertanda32_t pd_fisik;
    tak_bertanda32_t i;

    /* Alokasikan satu halaman untuk page directory */
    pd_fisik = (tak_bertanda32_t)hal_memory_alloc_page();
    if (pd_fisik == 0) {
        return 0;
    }

    /* Map ke virtual address */
    pd = (tak_bertanda32_t *)(uintptr_t)(pd_fisik + KERNEL_VIRTUAL_BASE);

    /* Clear page directory */
    kernel_memset(pd, 0, UKURAN_HALAMAN);

    /* Copy kernel mapping (entry 768+) */
    for (i = 768; i < 1024; i++) {
        tak_bertanda32_t *kernel_pd;
        kernel_pd = (tak_bertanda32_t *)(uintptr_t)cpu_read_cr3();
        pd[i] = kernel_pd[i];
    }

    return pd_fisik;
}

/*
 * _hancurkan_page_directory
 * -------------------------
 * Menghancurkan page directory proses.
 *
 * Parameter:
 *   pd_fisik - Alamat fisik page directory
 */
static void _hancurkan_page_directory(tak_bertanda32_t pd_fisik)
{
    if (pd_fisik == 0) {
        return;
    }

    hal_memory_free_page((alamat_fisik_t)pd_fisik);
}

/*
 * ============================================================================
 * FUNGSI INTERNAL - STACK
 * ============================================================================
 */

/*
 * _alokasi_stack_kernel
 * ---------------------
 * Mengalokasikan stack untuk kernel proses.
 *
 * Return:
 *   Alamat top stack, atau 0 jika gagal
 */
static tak_bertanda32_t _alokasi_stack_kernel(void)
{
    alamat_fisik_t halaman;
    tak_bertanda8_t *stack;

    /* Alokasikan stack kernel - gunakan konstanta dari konstanta.h */
    halaman = hal_memory_alloc_pages(2);
    if (halaman == ALAMAT_FISIK_INVALID) {
        return 0;
    }

    /* Map ke virtual address */
    stack = (tak_bertanda8_t *)(uintptr_t)(halaman + KERNEL_VIRTUAL_BASE);

    return (tak_bertanda32_t)(uintptr_t)(stack + (2 * UKURAN_HALAMAN));
}

/*
 * _bebaskan_stack_kernel
 * ----------------------
 * Membebaskan stack kernel.
 *
 * Parameter:
 *   stack_top - Alamat top stack
 */
static void _bebaskan_stack_kernel(tak_bertanda32_t stack_top)
{
    alamat_fisik_t halaman;

    if (stack_top == 0) {
        return;
    }

    halaman = (alamat_fisik_t)((tak_bertanda32_t)(stack_top - (2 * UKURAN_HALAMAN)) -
             KERNEL_VIRTUAL_BASE);
    hal_memory_free_pages(halaman, 2);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - INISIALISASI
 * ============================================================================
 */

/*
 * proses_x86_init
 * ---------------
 * Menginisialisasi subsistem proses x86.
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t proses_x86_init(void)
{
    kernel_printf("[PROSES-x86] Menginisialisasi subsistem proses...\n");

    /* Clear tabel proses */
    kernel_memset(&g_tabel_proses, 0, sizeof(g_tabel_proses));
    kernel_memset(&g_proses_kernel, 0, sizeof(g_proses_kernel));

    /* Init TSS */
    kernel_printf("[PROSES-x86] Inisialisasi TSS...\n");
    _tss_init();
    _tss_load();

    /* Setup proses kernel */
    kernel_printf("[PROSES-x86] Setup proses kernel...\n");
    g_proses_kernel.pid = PID_KERNEL;
    g_proses_kernel.ppid = 0;
    g_proses_kernel.status = PROSES_STATUS_JALAN;
    g_proses_kernel.prioritas = PRIORITAS_TINGGI;
    kernel_strncpy(g_proses_kernel.nama, "kernel", MAKS_NAMA_PROSES);
    g_proses_kernel.context.cr3 = cpu_read_cr3();
    g_proses_kernel.sedang_jalan = BENAR;
    g_proses_kernel.waktu_mulai = 0;

    g_proses_sekarang_internal = &g_proses_kernel;
    g_jumlah_proses = 1;

    g_proses_diinisialisasi = BENAR;

    kernel_printf("[PROSES-x86] Subsistem proses siap\n");

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - PEMBUATAN PROSES
 * ============================================================================
 */

/*
 * proses_x86_buat
 * ---------------
 * Membuat proses baru.
 *
 * Parameter:
 *   nama   - Nama proses
 *   ppid   - PID parent
 *   flags  - Flag proses
 *
 * Return:
 *   PID proses baru, atau PID_INVALID jika gagal
 */
pid_t proses_x86_buat(const char *nama, pid_t ppid,
                      tak_bertanda32_t flags)
{
    struct proses_internal *proses;
    tanda32_t slot;
    pid_t pid;

    if (!g_proses_diinisialisasi) {
        return PID_INVALID;
    }

    if (g_jumlah_proses >= MAKS_PROSES_SISTEM) {
        kernel_printf("[PROSES-x86] Tabel proses penuh\n");
        return PID_INVALID;
    }

    /* Cari slot kosong */
    slot = _alokasi_proses_slot();
    if (slot < 0) {
        kernel_printf("[PROSES-x86] Tidak ada slot kosong\n");
        return PID_INVALID;
    }

    proses = &g_tabel_proses[slot];

    /* Alokasi PID */
    pid = _alokasi_pid();
    if (pid == PID_INVALID) {
        return PID_INVALID;
    }

    /* Setup proses */
    proses->pid = pid;
    proses->ppid = ppid;
    proses->status = PROSES_STATUS_SIAP;
    proses->prioritas = PRIORITAS_NORMAL;
    proses->sedang_jalan = SALAH;
    proses->waktu_mulai = hal_timer_get_ticks();
    proses->waktu_cpu = 0;

    /* Copy nama */
    if (nama != NULL) {
        kernel_strncpy(proses->nama, nama, MAKS_NAMA_PROSES - 1);
        proses->nama[MAKS_NAMA_PROSES - 1] = '\0';
    } else {
        kernel_snprintf(proses->nama, MAKS_NAMA_PROSES,
                        "proses_%u", pid);
    }

    /* Alokasi stack kernel */
    proses->kernel_stack = _alokasi_stack_kernel();
    if (proses->kernel_stack == 0) {
        _dealokasi_proses(proses);
        kernel_printf("[PROSES-x86] Gagal alokasi stack kernel\n");
        return PID_INVALID;
    }

    /* Buat page directory untuk user process */
    if (flags & 0x01) {
        /* User mode flag */
        proses->pd = (tak_bertanda32_t *)(uintptr_t)_buat_page_directory();
        if (proses->pd == NULL) {
            _bebaskan_stack_kernel(proses->kernel_stack);
            _dealokasi_proses(proses);
            kernel_printf("[PROSES-x86] Gagal buat page directory\n");
            return PID_INVALID;
        }
    } else {
        /* Kernel process */
        proses->pd = NULL;
        proses->context.cr3 = cpu_read_cr3();
    }

    /* Set parent jika ada */
    if (ppid > 0) {
        struct proses_internal *parent;
        tak_bertanda32_t i;

        for (i = 0; i < MAKS_PROSES_SISTEM; i++) {
            if (g_tabel_proses[i].pid == ppid) {
                parent = &g_tabel_proses[i];
                proses->parent = parent;
                break;
            }
        }
    }

    g_jumlah_proses++;

    kernel_printf("[PROSES-x86] Proses baru: %s (PID: %u)\n",
                  proses->nama, proses->pid);

    return pid;
}

/*
 * proses_x86_buat_dengan_entry
 * ----------------------------
 * Membuat proses baru dengan entry point tertentu.
 *
 * Parameter:
 *   nama   - Nama proses
 *   entry  - Entry point
 *   ppid   - PID parent
 *   user   - BENAR untuk user mode
 *
 * Return:
 *   PID proses baru, atau PID_INVALID jika gagal
 */
pid_t proses_x86_buat_dengan_entry(const char *nama,
                                    tak_bertanda32_t entry,
                                    pid_t ppid,
                                    bool_t user)
{
    struct proses_internal *proses;
    pid_t pid;
    tak_bertanda32_t i;
    tak_bertanda32_t flags = user ? 0x01 : 0x00;

    pid = proses_x86_buat(nama, ppid, flags);
    if (pid == PID_INVALID) {
        return PID_INVALID;
    }

    /* Cari proses yang baru dibuat */
    proses = NULL;
    for (i = 0; i < MAKS_PROSES_SISTEM; i++) {
        if (g_tabel_proses[i].pid == pid) {
            proses = &g_tabel_proses[i];
            break;
        }
    }

    if (proses == NULL) {
        return PID_INVALID;
    }

    /* Init context */
    if (user) {
        tak_bertanda32_t pd_fisik;
        tak_bertanda32_t user_stack;

        /* Alokasi stack user */
        user_stack = ALAMAT_STACK_USER;

        pd_fisik = (tak_bertanda32_t)(uintptr_t)proses->pd;
        _context_init_user(proses, entry, user_stack, pd_fisik);
    } else {
        _context_init_kernel(proses, entry, proses->kernel_stack);
    }

    return pid;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - PENGHAPUSAN PROSES
 * ============================================================================
 */

/*
 * proses_x86_exit
 * ---------------
 * Mengakhiri proses.
 *
 * Parameter:
 *   pid       - PID proses
 *   exit_code - Kode exit
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t proses_x86_exit(pid_t pid, tanda32_t exit_code)
{
    struct proses_internal *proses;
    tak_bertanda32_t i;

    if (!g_proses_diinisialisasi) {
        return STATUS_GAGAL;
    }

    if (pid == PID_KERNEL) {
        kernel_printf("[PROSES-x86] Tidak dapat exit kernel\n");
        return STATUS_GAGAL;
    }

    /* Cari proses */
    proses = NULL;
    for (i = 0; i < MAKS_PROSES_SISTEM; i++) {
        if (g_tabel_proses[i].pid == pid) {
            proses = &g_tabel_proses[i];
            break;
        }
    }

    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    kernel_printf("[PROSES-x86] Exit proses %s (PID: %u, code: %d)\n",
                  proses->nama, proses->pid, exit_code);

    /* Bebaskan resources */
    if (proses->kernel_stack != 0) {
        _bebaskan_stack_kernel(proses->kernel_stack);
    }

    if (proses->pd != NULL) {
        _hancurkan_page_directory((tak_bertanda32_t)(uintptr_t)proses->pd);
    }

    /* Update status */
    proses->status = PROSES_STATUS_ZOMBIE;

    /* Jika ini proses saat ini, switch ke proses lain */
    if (g_proses_sekarang_internal == proses) {
        g_proses_sekarang_internal = &g_proses_kernel;
        g_proses_sekarang = (proses_t *)&g_proses_kernel;
    }

    g_jumlah_proses--;

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - CONTEXT SWITCHING
 * ============================================================================
 */

/*
 * proses_x86_switch
 * -----------------
 * Melakukan context switch ke proses lain.
 *
 * Parameter:
 *   pid_target - PID proses target, atau 0 untuk scheduler
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t proses_x86_switch(pid_t pid_target)
{
    struct proses_internal *proses_lama;
    struct proses_internal *proses_baru;
    tak_bertanda32_t i;

    if (!g_proses_diinisialisasi) {
        return STATUS_GAGAL;
    }

    proses_lama = g_proses_sekarang_internal;

    /* Cari proses target */
    if (pid_target == 0) {
        /* Simple round-robin scheduling */
        bool_t found = SALAH;

        for (i = 0; i < MAKS_PROSES_SISTEM; i++) {
            tak_bertanda32_t idx;

            idx = ((proses_lama - g_tabel_proses) + i + 1) %
                  MAKS_PROSES_SISTEM;

            if (g_tabel_proses[idx].status == PROSES_STATUS_SIAP ||
                g_tabel_proses[idx].status == PROSES_STATUS_JALAN) {
                proses_baru = &g_tabel_proses[idx];
                found = BENAR;
                break;
            }
        }

        if (!found) {
            /* Tidak ada proses lain, kembali ke kernel */
            proses_baru = &g_proses_kernel;
        }
    } else {
        /* Cari proses dengan PID tertentu */
        proses_baru = NULL;
        for (i = 0; i < MAKS_PROSES_SISTEM; i++) {
            if (g_tabel_proses[i].pid == pid_target &&
                (g_tabel_proses[i].status == PROSES_STATUS_SIAP ||
                 g_tabel_proses[i].status == PROSES_STATUS_JALAN)) {
                proses_baru = &g_tabel_proses[i];
                break;
            }
        }

        if (proses_baru == NULL) {
            return STATUS_TIDAK_DITEMUKAN;
        }
    }

    /* Tidak perlu switch jika proses sama */
    if (proses_lama == proses_baru) {
        return STATUS_BERHASIL;
    }

    /* Update status */
    if (proses_lama->status == PROSES_STATUS_JALAN) {
        proses_lama->status = PROSES_STATUS_SIAP;
    }
    proses_lama->sedang_jalan = SALAH;

    proses_baru->status = PROSES_STATUS_JALAN;
    proses_baru->sedang_jalan = BENAR;

    /* Update TSS dengan stack kernel proses baru */
    _tss_set_kernel_stack(proses_baru->kernel_stack);

    /* Update current process */
    g_proses_sekarang_internal = proses_baru;

    /* Update global pointer dari kernel.h (cast untuk kompatibilitas) */
    g_proses_sekarang = (proses_t *)proses_baru;

    /* Context switch */
    _context_switch(&proses_lama->context, &proses_baru->context);

    return STATUS_BERHASIL;
}

/*
 * proses_x86_yield
 * ----------------
 * Menyerahkan CPU ke proses lain.
 */
void proses_x86_yield(void)
{
    proses_x86_switch(0);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - INFORMASI PROSES
 * ============================================================================
 */

/*
 * proses_x86_get_sekarang
 * -----------------------
 * Mendapatkan proses saat ini.
 *
 * Return:
 *   Pointer ke struktur proses, atau NULL
 */
struct proses_internal *proses_x86_get_sekarang(void)
{
    return g_proses_sekarang_internal;
}

/*
 * proses_x86_get_pid_sekarang
 * ---------------------------
 * Mendapatkan PID proses saat ini.
 *
 * Return:
 *   PID proses saat ini
 */
pid_t proses_x86_get_pid_sekarang(void)
{
    if (g_proses_sekarang_internal == NULL) {
        return PID_KERNEL;
    }

    return g_proses_sekarang_internal->pid;
}

/*
 * proses_x86_cari
 * ---------------
 * Mencari proses berdasarkan PID.
 *
 * Parameter:
 *   pid - PID yang dicari
 *
 * Return:
 *   Pointer ke struktur proses, atau NULL
 */
struct proses_internal *proses_x86_cari(pid_t pid)
{
    tak_bertanda32_t i;

    for (i = 0; i < MAKS_PROSES_SISTEM; i++) {
        if (g_tabel_proses[i].pid == pid) {
            return &g_tabel_proses[i];
        }
    }

    return NULL;
}

/*
 * proses_x86_get_jumlah
 * ---------------------
 * Mendapatkan jumlah proses aktif.
 *
 * Return:
 *   Jumlah proses aktif
 */
tak_bertanda32_t proses_x86_get_jumlah(void)
{
    return g_jumlah_proses;
}

/*
 * proses_x86_print_daftar
 * -----------------------
 * Menampilkan daftar proses.
 */
void proses_x86_print_daftar(void)
{
    tak_bertanda32_t i;

    kernel_printf("\n=== Daftar Proses ===\n");
    kernel_printf("PID   PPID  STATUS    NAMA\n");
    kernel_printf("----  ----  --------  ----\n");

    /* Kernel process */
    kernel_printf("%4u  %4u  %-8s  %s\n",
                  g_proses_kernel.pid,
                  g_proses_kernel.ppid,
                  "JALAN",
                  g_proses_kernel.nama);

    /* User processes */
    for (i = 0; i < MAKS_PROSES_SISTEM; i++) {
        struct proses_internal *p = &g_tabel_proses[i];

        if (p->pid != 0 && p->status != PROSES_STATUS_BELUM) {
            const char *status_str;

            switch (p->status) {
                case PROSES_STATUS_SIAP:
                    status_str = "SIAP";
                    break;
                case PROSES_STATUS_JALAN:
                    status_str = "JALAN";
                    break;
                case PROSES_STATUS_TUNGGU:
                    status_str = "TUNGGU";
                    break;
                case PROSES_STATUS_ZOMBIE:
                    status_str = "ZOMBIE";
                    break;
                case PROSES_STATUS_STOP:
                    status_str = "STOP";
                    break;
                default:
                    status_str = "?";
                    break;
            }

            kernel_printf("%4u  %4u  %-8s  %s\n",
                          p->pid, p->ppid, status_str, p->nama);
        }
    }

    kernel_printf("\nTotal: %u proses\n", g_jumlah_proses);
    kernel_printf("====================\n");
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - USER MODE
 * ============================================================================
 */

/*
 * proses_x86_jump_usermode
 * ------------------------
 * Melompat ke user mode.
 *
 * Parameter:
 *   entry - Entry point di user mode
 *   stack - Stack pointer di user mode
 */
void proses_x86_jump_usermode(tak_bertanda32_t entry,
                               tak_bertanda32_t stack)
{
    _tss_set_kernel_stack(g_proses_sekarang_internal->kernel_stack);
    _jump_to_usermode(entry, stack);
}

/*
 * ============================================================================
 * FUNGSI PUBLIC - SETTER/GETTER
 * ============================================================================
 */

/*
 * proses_x86_set_status
 * ---------------------
 * Mengatur status proses.
 *
 * Parameter:
 *   pid    - PID proses
 *   status - Status baru
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t proses_x86_set_status(pid_t pid, proses_status_t status)
{
    struct proses_internal *proses;

    proses = proses_x86_cari(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    proses->status = status;

    return STATUS_BERHASIL;
}

/*
 * proses_x86_set_prioritas
 * ------------------------
 * Mengatur prioritas proses.
 *
 * Parameter:
 *   pid       - PID proses
 *   prioritas - Prioritas baru
 *
 * Return:
 *   STATUS_BERHASIL jika berhasil
 */
status_t proses_x86_set_prioritas(pid_t pid, prioritas_t prioritas)
{
    struct proses_internal *proses;

    proses = proses_x86_cari(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    proses->prioritas = prioritas;

    return STATUS_BERHASIL;
}

/*
 * proses_x86_apakah_siap
 * ----------------------
 * Mengecek apakah subsistem proses sudah diinisialisasi.
 *
 * Return:
 *   BENAR jika sudah, SALAH jika belum
 */
bool_t proses_x86_apakah_siap(void)
{
    return g_proses_diinisialisasi;
}
