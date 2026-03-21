/*
 * PIGURA OS - PROSES_ARM.C
 * -------------------------
 * Implementasi manajemen proses untuk arsitektur ARM (32-bit).
 *
 * Berkas ini berisi fungsi-fungsi untuk mengelola proses dan thread
 * pada prosesor ARM, termasuk context switching dan scheduler.
 *
 * Arsitektur: ARM (ARMv4/v5/v6 - 32-bit)
 * Versi: 1.0
 */

#include "../../inti/types.h"
#include "../../inti/konstanta.h"
#include "../../inti/hal/hal.h"

/*
 * ============================================================================
 * KONSTANTA LOKAL (LOCAL CONSTANTS)
 * ============================================================================
 */

/* CPSR mode bits */
#define CPSR_MODE_USR           0x10
#define CPSR_MODE_FIQ           0x11
#define CPSR_MODE_IRQ           0x12
#define CPSR_MODE_SVC           0x13
#define CPSR_MODE_ABT           0x17
#define CPSR_MODE_UND           0x1B
#define CPSR_MODE_SYS           0x1F

/* CPSR flags */
#define CPSR_IRQ_DISABLE        (1 << 7)
#define CPSR_FIQ_DISABLE        (1 << 6)

/* Stack sizes */
#define UKURAN_STACK_THREAD     (8UL * 1024UL)
#define UKURAN_STACK_KERNEL     (16UL * 1024UL)

/* Process limits */
#define MAKS_PROSES_INTERNAL    64
#define MAKS_THREAD_INTERNAL    256

/*
 * ============================================================================
 * STRUKTUR DATA (DATA STRUCTURES)
 * ============================================================================
 */

/* CPU context untuk ARM */
typedef struct {
    /* General purpose registers */
    tak_bertanda32_t r0;
    tak_bertanda32_t r1;
    tak_bertanda32_t r2;
    tak_bertanda32_t r3;
    tak_bertanda32_t r4;
    tak_bertanda32_t r5;
    tak_bertanda32_t r6;
    tak_bertanda32_t r7;
    tak_bertanda32_t r8;
    tak_bertanda32_t r9;
    tak_bertanda32_t r10;
    tak_bertanda32_t r11;
    tak_bertanda32_t r12;
    tak_bertanda32_t sp;
    tak_bertanda32_t lr;
    tak_bertanda32_t pc;
    tak_bertanda32_t cpsr;
    tak_bertanda32_t spsr;
} cpu_context_t;

/* Process control block */
typedef struct proses_pcb {
    pid_t pid;
    pid_t ppid;
    tid_t tid;
    char nama[MAKS_NAMA_PROSES];

    proses_status_t status;
    prioritas_t prioritas;

    /* Memory */
    alamat_virtual_t entry_point;
    alamat_virtual_t stack_top;
    alamat_virtual_t brk;
    alamat_fisik_t page_table;

    /* Scheduling */
    tak_bertanda32_t quantum;
    tak_bertanda32_t time_used;
    tak_bertanda64_t start_time;

    /* CPU context */
    cpu_context_t context;

    /* Flags */
    bool_t digunakan;
    bool_t is_kernel;

    /* Parent/child */
    struct proses_pcb *parent;
    struct proses_pcb *child;
    struct proses_pcb *sibling;

    /* Exit code */
    tak_bertanda32_t exit_code;
} proses_pcb_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Process table */
static proses_pcb_t g_process_table[MAKS_PROSES_INTERNAL];

/* Current running process */
static proses_pcb_t *g_current_process = NULL;

/* Idle process */
static proses_pcb_t g_idle_process;

/* Process count */
static tak_bertanda32_t g_process_count = 0;
static tak_bertanda32_t g_next_pid = 1;

/* Flag inisialisasi */
static bool_t g_process_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI BANTUAN (HELPER FUNCTIONS)
 * ============================================================================
 */

/*
 * proses_cari_slot_kosong
 * -----------------------
 * Cari slot kosong dalam process table.
 *
 * Return: Index slot, atau -1 jika penuh
 */
static tanda32_t proses_cari_slot_kosong(void)
{
    tak_bertanda32_t i;

    for (i = 0; i < MAKS_PROSES_INTERNAL; i++) {
        if (!g_process_table[i].digunakan) {
            return (tanda32_t)i;
        }
    }

    return -1;
}

/*
 * proses_cari_dengan_pid
 * ----------------------
 * Cari proses berdasarkan PID.
 *
 * Parameter:
 *   pid - PID yang dicari
 *
 * Return: Pointer ke PCB, atau NULL jika tidak ditemukan
 */
static proses_pcb_t *proses_cari_dengan_pid(pid_t pid)
{
    tak_bertanda32_t i;

    for (i = 0; i < MAKS_PROSES_INTERNAL; i++) {
        if (g_process_table[i].digunakan && 
            g_process_table[i].pid == pid) {
            return &g_process_table[i];
        }
    }

    return NULL;
}

/*
 * ============================================================================
 * FUNGSI CONTEXT SWITCH (CONTEXT SWITCH FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_proses_context_save
 * -----------------------
 * Simpan context proses saat ini.
 *
 * Parameter:
 *   context - Pointer ke struktur context
 */
void hal_proses_context_save(cpu_context_t *context)
{
    if (g_current_process == NULL) {
        return;
    }

    kernel_memcpy(&g_current_process->context, context, sizeof(cpu_context_t));
}

/*
 * hal_proses_context_restore
 * --------------------------
 * Restore context proses.
 *
 * Parameter:
 *   context - Pointer ke struktur context
 */
void hal_proses_context_restore(cpu_context_t *context)
{
    if (g_current_process == NULL) {
        return;
    }

    kernel_memcpy(context, &g_current_process->context, sizeof(cpu_context_t));
}

/*
 * hal_proses_context_switch
 * -------------------------
 * Switch context ke proses baru.
 *
 * Parameter:
 *   proses_baru - Pointer ke PCB proses baru
 *
 * Return: Status operasi
 */
status_t hal_proses_context_switch(proses_pcb_t *proses_baru)
{
    tak_bertanda32_t cpsr;

    if (proses_baru == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Disable interrupts during context switch */
    cpsr = hal_cpu_save_interrupts();

    /* Update current process */
    g_current_process = proses_baru;

    /* Switch page table jika proses user */
    if (!proses_baru->is_kernel && proses_baru->page_table != 0) {
        hal_memori_switch_page_table(proses_baru->page_table);
    }

    /* Restore interrupts */
    hal_cpu_restore_interrupts(cpsr);

    return STATUS_BERHASIL;
}

/*
 * hal_proses_switch_to
 * --------------------
 * Switch ke proses tertentu.
 *
 * Parameter:
 *   pid - PID proses target
 *
 * Return: Status operasi
 */
status_t hal_proses_switch_to(pid_t pid)
{
    proses_pcb_t *proses;

    proses = proses_cari_dengan_pid(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (proses->status != PROSES_STATUS_SIAP) {
        return STATUS_PARAM_INVALID;
    }

    proses->status = PROSES_STATUS_JALAN;
    return hal_proses_context_switch(proses);
}

/*
 * ============================================================================
 * FUNGSI PEMBUATAN PROSES (PROCESS CREATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_proses_buat
 * ---------------
 * Buat proses baru.
 *
 * Parameter:
 *   nama        - Nama proses
 *   entry_point - Entry point proses
 *   is_kernel   - Apakah proses kernel
 *
 * Return: PID proses baru, atau -1 jika gagal
 */
pid_t hal_proses_buat(const char *nama, alamat_virtual_t entry_point,
    bool_t is_kernel)
{
    tanda32_t slot;
    proses_pcb_t *proses;
    tak_bertanda32_t i;

    slot = proses_cari_slot_kosong();
    if (slot < 0) {
        return PID_INVALID;
    }

    proses = &g_process_table[slot];

    /* Clear PCB */
    kernel_memset(proses, 0, sizeof(proses_pcb_t));

    /* Set PID */
    proses->pid = g_next_pid++;
    proses->tid = (tid_t)proses->pid;

    /* Set nama */
    if (nama != NULL) {
        for (i = 0; i < MAKS_NAMA_PROSES - 1 && nama[i] != '\0'; i++) {
            proses->nama[i] = nama[i];
        }
        proses->nama[i] = '\0';
    }

    /* Set entry point */
    proses->entry_point = entry_point;

    /* Alokasikan stack */
    if (is_kernel) {
        proses->stack_top = (alamat_virtual_t)hal_memori_phys_alloc_pages(
            UKURAN_STACK_KERNEL / UKURAN_HALAMAN);
        if (proses->stack_top == 0) {
            return PID_INVALID;
        }
    } else {
        proses->stack_top = 0;
    }

    /* Set status */
    proses->status = PROSES_STATUS_SIAP;
    proses->prioritas = PRIORITAS_NORMAL;
    proses->quantum = QUANTUM_DEFAULT;

    /* Set flags */
    proses->digunakan = BENAR;
    proses->is_kernel = is_kernel;

    /* Setup initial context */
    proses->context.pc = (tak_bertanda32_t)entry_point;
    proses->context.sp = (tak_bertanda32_t)(proses->stack_top +
        (is_kernel ? UKURAN_STACK_KERNEL : UKURAN_STACK_USER) - 16);
    proses->context.lr = 0;

    /* Set CPSR */
    if (is_kernel) {
        proses->context.cpsr = CPSR_MODE_SVC | CPSR_FIQ_DISABLE;
    } else {
        proses->context.cpsr = CPSR_MODE_USR;
    }

    g_process_count++;

    return proses->pid;
}

/*
 * hal_proses_hapus
 * ----------------
 * Hapus proses.
 *
 * Parameter:
 *   pid - PID proses
 *
 * Return: Status operasi
 */
status_t hal_proses_hapus(pid_t pid)
{
    proses_pcb_t *proses;

    proses = proses_cari_dengan_pid(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Bebaskan stack jika ada */
    if (proses->stack_top != 0) {
        tak_bertanda32_t pages = (proses->is_kernel ?
            UKURAN_STACK_KERNEL : UKURAN_STACK_USER) / UKURAN_HALAMAN;
        hal_memori_phys_free_pages(proses->stack_top, pages);
    }

    /* Bebaskan page table jika ada */
    if (proses->page_table != 0) {
        hal_memori_phys_free_page(proses->page_table);
    }

    proses->digunakan = SALAH;
    g_process_count--;

    return STATUS_BERHASIL;
}

/*
 * hal_proses_exit
 * ---------------
 * Exit proses dengan kode tertentu.
 *
 * Parameter:
 *   exit_code - Kode exit
 */
void hal_proses_exit(tak_bertanda32_t exit_code)
{
    if (g_current_process == NULL) {
        return;
    }

    g_current_process->exit_code = exit_code;
    g_current_process->status = PROSES_STATUS_ZOMBIE;

    hal_proses_schedule();
}

/*
 * ============================================================================
 * FUNGSI SCHEDULER (SCHEDULER FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_proses_schedule
 * -------------------
 * Jalankan scheduler untuk memilih proses berikutnya.
 */
void hal_proses_schedule(void)
{
    proses_pcb_t *next = NULL;
    tak_bertanda32_t i;
    tak_bertanda32_t highest_priority = 0;

    /* Cari proses dengan prioritas tertinggi */
    for (i = 0; i < MAKS_PROSES_INTERNAL; i++) {
        proses_pcb_t *p = &g_process_table[i];

        if (p->digunakan && p->status == PROSES_STATUS_SIAP) {
            if (next == NULL || p->prioritas > highest_priority) {
                next = p;
                highest_priority = p->prioritas;
            }
        }
    }

    /* Jika tidak ada proses siap, gunakan idle */
    if (next == NULL) {
        next = &g_idle_process;
    }

    /* Set current process ke ready jika masih running */
    if (g_current_process != NULL &&
        g_current_process->status == PROSES_STATUS_JALAN) {
        g_current_process->status = PROSES_STATUS_SIAP;
    }

    /* Switch ke proses berikutnya */
    next->status = PROSES_STATUS_JALAN;
    hal_proses_context_switch(next);
}

/*
 * hal_proses_get_current
 * ----------------------
 * Dapatkan proses saat ini.
 *
 * Return: Pointer ke PCB proses saat ini
 */
proses_pcb_t *hal_proses_get_current(void)
{
    return g_current_process;
}

/*
 * hal_proses_get_pid
 * ------------------
 * Dapatkan PID proses saat ini.
 *
 * Return: PID proses saat ini
 */
pid_t hal_proses_get_pid(void)
{
    if (g_current_process == NULL) {
        return PID_INVALID;
    }

    return g_current_process->pid;
}

/*
 * ============================================================================
 * FUNGSI INISIALISASI (INITIALIZATION FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_proses_buat_idle
 * --------------------
 * Buat proses idle.
 *
 * Return: Status operasi
 */
static status_t hal_proses_buat_idle(void)
{
    kernel_memset(&g_idle_process, 0, sizeof(proses_pcb_t));

    g_idle_process.pid = 0;
    g_idle_process.tid = 0;
    kernel_strcpy(g_idle_process.nama, "idle");

    g_idle_process.status = PROSES_STATUS_SIAP;
    g_idle_process.prioritas = PRIORITAS_RENDAH;
    g_idle_process.quantum = 1;

    g_idle_process.digunakan = BENAR;
    g_idle_process.is_kernel = BENAR;

    g_idle_process.context.pc = (tak_bertanda32_t)hal_cpu_halt;
    g_idle_process.context.cpsr = CPSR_MODE_SVC | CPSR_IRQ_DISABLE;

    return STATUS_BERHASIL;
}

/*
 * hal_proses_arch_init
 * --------------------
 * Inisialisasi subsistem proses arsitektur ARM.
 *
 * Return: Status operasi
 */
status_t hal_proses_arch_init(void)
{
    tak_bertanda32_t i;

    /* Clear process table */
    for (i = 0; i < MAKS_PROSES_INTERNAL; i++) {
        kernel_memset(&g_process_table[i], 0, sizeof(proses_pcb_t));
        g_process_table[i].digunakan = SALAH;
    }

    /* Buat idle process */
    hal_proses_buat_idle();

    /* Set idle sebagai current process */
    g_current_process = &g_idle_process;

    g_process_count = 0;
    g_next_pid = 1;
    g_process_initialized = BENAR;

    return STATUS_BERHASIL;
}

/*
 * hal_proses_arch_shutdown
 * ------------------------
 * Shutdown subsistem proses.
 *
 * Return: Status operasi
 */
status_t hal_proses_arch_shutdown(void)
{
    tak_bertanda32_t i;

    /* Hapus semua proses */
    for (i = 0; i < MAKS_PROSES_INTERNAL; i++) {
        if (g_process_table[i].digunakan) {
            hal_proses_hapus(g_process_table[i].pid);
        }
    }

    g_process_initialized = SALAH;

    return STATUS_BERHASIL;
}

/*
 * ============================================================================
 * FUNGSI UTILITAS (UTILITY FUNCTIONS)
 * ============================================================================
 */

/*
 * hal_proses_set_status
 * ---------------------
 * Set status proses.
 *
 * Parameter:
 *   pid    - PID proses
 *   status - Status baru
 *
 * Return: Status operasi
 */
status_t hal_proses_set_status(pid_t pid, proses_status_t status)
{
    proses_pcb_t *proses;

    proses = proses_cari_dengan_pid(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    proses->status = status;
    return STATUS_BERHASIL;
}

/*
 * hal_proses_set_prioritas
 * ------------------------
 * Set prioritas proses.
 *
 * Parameter:
 *   pid       - PID proses
 *   prioritas - Prioritas baru
 *
 * Return: Status operasi
 */
status_t hal_proses_set_prioritas(pid_t pid, prioritas_t prioritas)
{
    proses_pcb_t *proses;

    proses = proses_cari_dengan_pid(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    proses->prioritas = prioritas;
    return STATUS_BERHASIL;
}

/*
 * hal_proses_get_info
 * -------------------
 * Dapatkan informasi proses.
 *
 * Parameter:
 *   pid  - PID proses
 *   info - Pointer ke buffer info
 *
 * Return: Status operasi
 */
status_t hal_proses_get_info(pid_t pid, void *info)
{
    proses_pcb_t *proses;

    proses = proses_cari_dengan_pid(pid);
    if (proses == NULL) {
        return STATUS_TIDAK_DITEMUKAN;
    }

    if (info != NULL) {
        kernel_memcpy(info, proses, sizeof(proses_pcb_t));
    }

    return STATUS_BERHASIL;
}

/*
 * hal_proses_get_count
 * --------------------
 * Dapatkan jumlah proses.
 *
 * Return: Jumlah proses
 */
tak_bertanda32_t hal_proses_get_count(void)
{
    return g_process_count;
}
