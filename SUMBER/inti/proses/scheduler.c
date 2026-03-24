/*
 * PIGURA OS - SCHEDULER.C
 * ------------------------
 * Implementasi penjadwalan proses dengan algoritma multi-level
 * feedback queue dan priority scheduling.
 *
 * Berkas ini berisi implementasi scheduler preemptive dengan
 * dukungan untuk prioritas, real-time scheduling, dan load
 * balancing antar CPU.
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

/* Jumlah antrian prioritas */
#define JUMLAH_ANTRIAN         SCHEDULER_ANTRIAN_JUMLAH

/* Interval untuk priority boost (dalam ticks) */
#define BOOST_INTERVAL         100

/* Interval untuk load average calculation */
#define LOAD_AVG_INTERVAL      500

/* Load average decay factor (fixed point) */
#define LOAD_AVG_FACTOR        1884    /* e^(5sec/60sec) * 2048 */
#define LOAD_AVG_SHIFT         11

/* Timeslice bonus untuk interactive tasks */
#define TIMESLICE_BONUS        5
#define INTERACTIVE_THRESHOLD  100

/* Starvation threshold */
#define STARVATION_THRESHOLD   500

/* CPU IDLE threshold */
#define CPU_IDLE_THRESHOLD     10

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur untuk tracking CPU usage */
typedef struct {
    tak_bertanda64_t idle_ticks;
    tak_bertanda64_t total_ticks;
    tak_bertanda64_t last_update;
    tak_bertanda32_t load;
} cpu_usage_t;

/* Struktur untuk scheduling statistics per CPU */
typedef struct {
    tak_bertanda64_t context_switches;
    tak_bertanda64_t preemptions;
    tak_bertanda64_t yields;
    tak_bertanda64_t timeouts;
    tak_bertanda64_t migrations;
    tak_bertanda64_t wakeups;
    tak_bertanda64_t sleeps;
} cpu_sched_stats_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Scheduler state per CPU */
static scheduler_t g_sched[CPU_MAKS];

/* CPU usage tracking */
static cpu_usage_t g_cpu_usage[CPU_MAKS];

/* CPU scheduling statistics */
static cpu_sched_stats_t g_cpu_stats[CPU_MAKS];

/* Global scheduling statistics */
static struct {
    tak_bertanda64_t total_switches;
    tak_bertanda64_t total_preemptions;
    tak_bertanda64_t total_yields;
    tak_bertanda64_t total_timeouts;
    tak_bertanda64_t total_migrations;
    tak_bertanda64_t boost_count;
    tak_bertanda64_t migration_attempts;
    tak_bertanda64_t load_balance_count;
} g_sched_stats = {0, 0, 0, 0, 0, 0, 0, 0};

/* Load average global */
static tak_bertanda32_t g_load_avg[3] = {0, 0, 0};

/* Status inisialisasi */
static bool_t g_scheduler_initialized = SALAH;

/* Idle threads per CPU */
static thread_t *g_idle_threads[CPU_MAKS];

/*
 * ============================================================================
 * FUNGSI INTERNAL RUNQUEUE (INTERNAL RUNQUEUE FUNCTIONS)
 * ============================================================================
 */

/*
 * runqueue_init
 * -------------
 * Inisialisasi runqueue.
 *
 * Parameter:
 *   rq - Pointer ke runqueue
 */
static void runqueue_init(runqueue_t *rq)
{
    if (rq == NULL) {
        return;
    }
    
    rq->head = NULL;
    rq->tail = NULL;
    rq->count = 0;
    rq->total_time = 0;
}

/*
 * runqueue_tambah
 * ---------------
 * Tambahkan thread ke akhir runqueue.
 *
 * Parameter:
 *   rq     - Pointer ke runqueue
 *   thread - Pointer ke thread
 */
static void runqueue_tambah(runqueue_t *rq, thread_t *thread)
{
    if (rq == NULL || thread == NULL) {
        return;
    }
    
    thread->next = NULL;
    thread->prev = NULL;
    
    if (rq->tail == NULL) {
        rq->head = thread;
        rq->tail = thread;
    } else {
        rq->tail->next = thread;
        thread->prev = rq->tail;
        rq->tail = thread;
    }
    
    rq->count++;
}

/*
 * runqueue_tambah_depan
 * ---------------------
 * Tambahkan thread ke depan runqueue.
 * CATATAN: Fungsi ini disiapkan untuk penggunaan di masa depan.
 *
 * Parameter:
 *   rq     - Pointer ke runqueue
 *   thread - Pointer ke thread
 */
static void runqueue_tambah_depan(runqueue_t *rq, thread_t *thread)
    __attribute__((unused));
static void runqueue_tambah_depan(runqueue_t *rq, thread_t *thread)
{
    if (rq == NULL || thread == NULL) {
        return;
    }
    
    thread->next = NULL;
    thread->prev = NULL;
    
    if (rq->head == NULL) {
        rq->head = thread;
        rq->tail = thread;
    } else {
        thread->next = rq->head;
        rq->head->prev = thread;
        rq->head = thread;
    }
    
    rq->count++;
}

/*
 * runqueue_hapus
 * --------------
 * Hapus thread dari runqueue.
 *
 * Parameter:
 *   rq     - Pointer ke runqueue
 *   thread - Pointer ke thread
 */
static void runqueue_hapus(runqueue_t *rq, thread_t *thread)
{
    if (rq == NULL || thread == NULL) {
        return;
    }
    
    if (thread->prev != NULL) {
        thread->prev->next = thread->next;
    } else {
        rq->head = thread->next;
    }
    
    if (thread->next != NULL) {
        thread->next->prev = thread->prev;
    } else {
        rq->tail = thread->prev;
    }
    
    thread->next = NULL;
    thread->prev = NULL;
    
    if (rq->count > 0) {
        rq->count--;
    }
}

/*
 * runqueue_ambil
 * --------------
 * Ambil thread pertama dari runqueue.
 *
 * Parameter:
 *   rq - Pointer ke runqueue
 *
 * Return: Pointer ke thread, atau NULL jika kosong
 */
static thread_t *runqueue_ambil(runqueue_t *rq)
{
    thread_t *thread;
    
    if (rq == NULL || rq->head == NULL) {
        return NULL;
    }
    
    thread = rq->head;
    runqueue_hapus(rq, thread);
    
    return thread;
}

/*
 * runqueue_peek
 * -------------
 * Lihat thread pertama tanpa menghapus.
 *
 * Parameter:
 *   rq - Pointer ke runqueue
 *
 * Return: Pointer ke thread, atau NULL
 */
static thread_t *runqueue_peek(runqueue_t *rq)
    __attribute__((unused));
static thread_t *runqueue_peek(runqueue_t *rq)
{
    if (rq == NULL) {
        return NULL;
    }
    
    return rq->head;
}

/*
 * ============================================================================
 * FUNGSI INTERNAL SCHEDULER (INTERNAL SCHEDULER FUNCTIONS)
 * ============================================================================
 */

/*
 * dapatkan_scheduler
 * ------------------
 * Dapatkan scheduler untuk CPU tertentu.
 *
 * Parameter:
 *   cpu_id - CPU ID
 *
 * Return: Pointer ke scheduler
 */
static scheduler_t *dapatkan_scheduler(tak_bertanda32_t cpu_id)
    __attribute__((unused));
static scheduler_t *dapatkan_scheduler(tak_bertanda32_t cpu_id)
{
    if (cpu_id >= CPU_MAKS) {
        cpu_id = 0;
    }
    
    return &g_sched[cpu_id];
}

/*
 * dapatkan_cpu_id
 * ---------------
 * Dapatkan CPU ID saat ini.
 *
 * Return: CPU ID
 */
static tak_bertanda32_t dapatkan_cpu_id(void)
{
    tak_bertanda32_t cpu_id;
    
    cpu_id = hal_cpu_get_id();
    
    if (cpu_id >= CPU_MAKS) {
        cpu_id = 0;
    }
    
    return cpu_id;
}

/*
 * hitung_load_avg
 * ---------------
 * Hitung load average.
 */
static void hitung_load_avg(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t total_runnable;
    tak_bertanda32_t new_load;
    scheduler_t *sched;
    
    total_runnable = 0;
    
    for (i = 0; i < hal_cpu_get_count() && i < CPU_MAKS; i++) {
        sched = &g_sched[i];
        total_runnable += sched->total_threads;
    }
    
    /* Calculate load average dengan exponential decay */
    new_load = (total_runnable * LOAD_AVG_FACTOR) >> LOAD_AVG_SHIFT;
    
    /* 1 minute average */
    g_load_avg[0] = ((g_load_avg[0] * 1884) + (new_load * 164)) >> 11;
    
    /* 5 minute average */
    g_load_avg[1] = ((g_load_avg[1] * 2017) + (new_load * 31)) >> 11;
    
    /* 15 minute average */
    g_load_avg[2] = ((g_load_avg[2] * 2037) + (new_load * 11)) >> 11;
}

/*
 * boost_prioritas
 * ---------------
 * Boost prioritas thread yang kelaparan.
 */
static void boost_prioritas(void)
{
    scheduler_t *sched;
    runqueue_t *rq;
    thread_t *thread;
    thread_t *next;
    tak_bertanda32_t cpu_id;
    tak_bertanda32_t i;
    tak_bertanda64_t current_time;
    
    current_time = hal_timer_get_ticks();
    
    for (cpu_id = 0; cpu_id < hal_cpu_get_count() && cpu_id < CPU_MAKS; cpu_id++) {
        sched = &g_sched[cpu_id];
        
        /* Cek apakah perlu boost */
        if (current_time - sched->last_boost < BOOST_INTERVAL) {
            continue;
        }
        
        /* Boost thread di antrian rendah ke antrian normal */
        for (i = PRIORITAS_RENDAH; i < JUMLAH_ANTRIAN; i++) {
            rq = &sched->antrian[i];
            
            thread = rq->head;
            while (thread != NULL) {
                next = thread->next;
                
                /* Cek starvation */
                if (current_time - thread->start_time > STARVATION_THRESHOLD) {
                    /* Pindahkan ke antrian normal */
                    runqueue_hapus(rq, thread);
                    
                    thread->prioritas = PRIORITAS_NORMAL;
                    thread->quantum = QUANTUM_NORMAL + TIMESLICE_BONUS;
                    thread->start_time = current_time;
                    
                    runqueue_tambah(&sched->antrian[PRIORITAS_NORMAL], thread);
                }
                
                thread = next;
            }
        }
        
        sched->last_boost = current_time;
    }
    
    g_sched_stats.boost_count++;
}

/*
 * load_balance_check
 * ------------------
 * Cek dan lakukan load balancing antar CPU.
 */
static void load_balance_check(void)
{
    tak_bertanda32_t cpu_count;
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda32_t max_load;
    tak_bertanda32_t min_load;
    tak_bertanda32_t max_cpu;
    tak_bertanda32_t min_cpu;
    scheduler_t *src_sched;
    scheduler_t *dst_sched;
    thread_t *thread;
    
    cpu_count = hal_cpu_get_count();
    if (cpu_count < 2 || cpu_count > CPU_MAKS) {
        (void)j;  /* Suppress unused variable warning */
        return;
    }
    
    /* Cari CPU dengan load tertinggi dan terendah */
    max_load = 0;
    min_load = 0xFFFFFFFF;
    max_cpu = 0;
    min_cpu = 0;
    
    for (i = 0; i < cpu_count; i++) {
        if (g_sched[i].total_threads > max_load) {
            max_load = g_sched[i].total_threads;
            max_cpu = i;
        }
        
        if (g_sched[i].total_threads < min_load) {
            min_load = g_sched[i].total_threads;
            min_cpu = i;
        }
    }
    
    /* Cek apakah perlu load balancing */
    if (max_load - min_load < 2) {
        return;
    }
    
    /* Migrasikan thread dari CPU dengan load tinggi */
    src_sched = &g_sched[max_cpu];
    dst_sched = &g_sched[min_cpu];
    
    /* Cari thread yang bisa dimigrasikan */
    for (i = 1; i < JUMLAH_ANTRIAN; i++) {
        if (src_sched->antrian[i].count > 0) {
            thread = runqueue_ambil(&src_sched->antrian[i]);
            if (thread != NULL) {
                /* Update CPU affinity */
                thread->last_cpu = min_cpu;
                
                /* Masukkan ke antrian baru */
                runqueue_tambah(&dst_sched->antrian[thread->prioritas],
                               thread);
                
                src_sched->total_threads--;
                dst_sched->total_threads++;
                
                g_cpu_stats[max_cpu].migrations++;
                g_cpu_stats[min_cpu].migrations++;
                g_sched_stats.total_migrations++;
                
                break;
            }
        }
    }
    
    g_sched_stats.load_balance_count++;
}

/*
 * pilih_thread_berikutnya
 * -----------------------
 * Pilih thread berikutnya untuk dijalankan.
 *
 * Parameter:
 *   sched - Pointer ke scheduler
 *
 * Return: Pointer ke thread, atau NULL
 */
static thread_t *pilih_thread_berikutnya(scheduler_t *sched)
{
    thread_t *thread;
    tak_bertanda32_t i;
    
    if (sched == NULL) {
        return NULL;
    }
    
    /* Prioritas realtime pertama */
    if (sched->rt_queue.count > 0) {
        thread = runqueue_ambil(&sched->rt_queue);
        if (thread != NULL) {
            return thread;
        }
    }
    
    /* Cari dari antrian prioritas tertinggi */
    for (i = 0; i < JUMLAH_ANTRIAN; i++) {
        if (sched->antrian[i].count > 0) {
            thread = runqueue_ambil(&sched->antrian[i]);
            if (thread != NULL) {
                return thread;
            }
        }
    }
    
    /* Tidak ada thread runnable, kembalikan idle */
    return sched->idle_thread;
}

/*
 * update_cpu_usage
 * ----------------
 * Update penggunaan CPU.
 *
 * Parameter:
 *   cpu_id - CPU ID
 *   idle   - Apakah CPU idle
 */
static void update_cpu_usage(tak_bertanda32_t cpu_id, bool_t idle)
{
    cpu_usage_t *usage;
    tak_bertanda64_t current_time;
    tak_bertanda64_t elapsed;
    
    if (cpu_id >= CPU_MAKS) {
        return;
    }
    
    usage = &g_cpu_usage[cpu_id];
    current_time = hal_timer_get_ticks();
    
    elapsed = current_time - usage->last_update;
    usage->total_ticks += elapsed;
    
    if (idle) {
        usage->idle_ticks += elapsed;
    }
    
    usage->last_update = current_time;
    
    /* Hitung load dalam persen */
    if (usage->total_ticks > 0) {
        usage->load = (tak_bertanda32_t)
            ((usage->total_ticks - usage->idle_ticks) * 100 /
             usage->total_ticks);
    }
}

/*
 * ============================================================================
 * FUNGSI PUBLIK SCHEDULER (PUBLIC SCHEDULER FUNCTIONS)
 * ============================================================================
 */

/*
 * scheduler_init
 * --------------
 * Inisialisasi subsistem scheduler.
 *
 * Return: Status operasi
 */
status_t scheduler_init(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t j;
    tak_bertanda32_t cpu_count;
    scheduler_t *sched;
    
    if (g_scheduler_initialized) {
        return STATUS_SUDAH_ADA;
    }
    
    /* Reset global state */
    kernel_memset(g_sched, 0, sizeof(g_sched));
    kernel_memset(g_cpu_usage, 0, sizeof(g_cpu_usage));
    kernel_memset(g_cpu_stats, 0, sizeof(g_cpu_stats));
    kernel_memset(&g_sched_stats, 0, sizeof(g_sched_stats));
    kernel_memset(g_load_avg, 0, sizeof(g_load_avg));
    kernel_memset(g_idle_threads, 0, sizeof(g_idle_threads));
    
    cpu_count = hal_cpu_get_count();
    if (cpu_count > CPU_MAKS) {
        cpu_count = CPU_MAKS;
    }
    
    /* Inisialisasi scheduler per CPU */
    for (i = 0; i < cpu_count; i++) {
        sched = &g_sched[i];
        
        /* Init antrian */
        for (j = 0; j < JUMLAH_ANTRIAN; j++) {
            runqueue_init(&sched->antrian[j]);
        }
        
        /* Init realtime queue */
        runqueue_init(&sched->rt_queue);
        runqueue_init(&sched->dl_queue);
        
        /* Init lock */
        spinlock_init(&sched->lock);
        
        /* Init flags */
        sched->flags = SCHED_FLAG_NONE;
        
        /* Init counters */
        sched->total_proses = 0;
        sched->total_threads = 0;
        sched->tick_count = 0;
        sched->last_boost = 0;
        sched->boost_interval = BOOST_INTERVAL;
        sched->cpu_id = i;
        sched->preempt_count = 0;
        sched->current_thread = NULL;
        sched->idle_thread = NULL;
        
        /* Init CPU usage */
        g_cpu_usage[i].idle_ticks = 0;
        g_cpu_usage[i].total_ticks = 0;
        g_cpu_usage[i].last_update = hal_timer_get_ticks();
        g_cpu_usage[i].load = 0;
    }
    
    g_scheduler_initialized = BENAR;
    
    kernel_printf("[SCHED] Scheduler initialized\n");
    kernel_printf("        CPUs: %lu, Queues: %d per CPU\n",
                  (tak_bertanda64_t)cpu_count, JUMLAH_ANTRIAN);
    kernel_printf("        Quantum: RT=%d, HI=%d, NM=%d, LO=%d\n",
                  QUANTUM_REALTIME, QUANTUM_TINGGI,
                  QUANTUM_NORMAL, QUANTUM_RENDAH);
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_init_cpu
 * ------------------
 * Inisialisasi scheduler untuk CPU tertentu.
 *
 * Parameter:
 *   cpu_id - CPU ID
 *
 * Return: Status operasi
 */
status_t scheduler_init_cpu(tak_bertanda32_t cpu_id)
{
    scheduler_t *sched;
    
    if (!g_scheduler_initialized) {
        return STATUS_GAGAL;
    }
    
    if (cpu_id >= CPU_MAKS) {
        return STATUS_PARAM_INVALID;
    }
    
    sched = &g_sched[cpu_id];
    
    /* Buat idle thread untuk CPU ini */
    g_idle_threads[cpu_id] = thread_alokasi();
    if (g_idle_threads[cpu_id] == NULL) {
        return STATUS_MEMORI_HABIS;
    }
    
    g_idle_threads[cpu_id]->pid = 0;
    g_idle_threads[cpu_id]->tid = cpu_id + 1000;  /* TID khusus idle */
    g_idle_threads[cpu_id]->status = THREAD_STATUS_SIAP;
    g_idle_threads[cpu_id]->flags = THREAD_FLAG_KERNEL;
    g_idle_threads[cpu_id]->prioritas = PRIORITAS_IDLE;
    g_idle_threads[cpu_id]->quantum = QUANTUM_IDLE;
    g_idle_threads[cpu_id]->cpu_affinity = (1UL << cpu_id);
    g_idle_threads[cpu_id]->last_cpu = cpu_id;
    
    sched->idle_thread = g_idle_threads[cpu_id];
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_tambah_thread
 * -----------------------
 * Tambahkan thread ke scheduler.
 *
 * Parameter:
 *   thread - Pointer ke thread
 *
 * Return: Status operasi
 */
status_t scheduler_tambah_thread(thread_t *thread)
{
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    tak_bertanda32_t idx;
    
    if (!g_scheduler_initialized || thread == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Tentukan CPU target */
    cpu_id = thread->last_cpu;
    if (cpu_id >= hal_cpu_get_count() || cpu_id >= CPU_MAKS) {
        cpu_id = dapatkan_cpu_id();
    }
    
    sched = &g_sched[cpu_id];
    
    spinlock_kunci(&sched->lock);
    
    /* Tentukan antrian berdasarkan prioritas */
    if (thread->prioritas >= PRIORITAS_REALTIME) {
        /* Realtime queue */
        runqueue_tambah(&sched->rt_queue, thread);
    } else {
        /* Antrian normal */
        idx = dapatkan_runqueue_idx(thread->prioritas);
        if (idx >= JUMLAH_ANTRIAN) {
            idx = JUMLAH_ANTRIAN - 1;
        }
        
        runqueue_tambah(&sched->antrian[idx], thread);
    }
    
    /* Reset quantum */
    thread->quantum = dapatkan_quantum(thread->prioritas);
    thread->status = THREAD_STATUS_SIAP;
    thread->start_time = hal_timer_get_ticks();
    
    sched->total_threads++;
    
    spinlock_buka(&sched->lock);
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_hapus_thread
 * ----------------------
 * Hapus thread dari scheduler.
 *
 * Parameter:
 *   thread - Pointer ke thread
 *
 * Return: Status operasi
 */
status_t scheduler_hapus_thread(thread_t *thread)
{
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    tak_bertanda32_t idx;
    bool_t found = SALAH;
    
    if (!g_scheduler_initialized || thread == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    cpu_id = thread->last_cpu;
    if (cpu_id >= CPU_MAKS) {
        cpu_id = 0;
    }
    
    sched = &g_sched[cpu_id];
    
    spinlock_kunci(&sched->lock);
    
    /* Cari di realtime queue */
    if (sched->rt_queue.count > 0) {
        runqueue_hapus(&sched->rt_queue, thread);
        found = BENAR;
    }
    
    /* Cari di antrian normal */
    if (!found) {
        for (idx = 0; idx < JUMLAH_ANTRIAN; idx++) {
            if (sched->antrian[idx].count > 0) {
                runqueue_hapus(&sched->antrian[idx], thread);
                found = BENAR;
                break;
            }
        }
    }
    
    if (found && sched->total_threads > 0) {
        sched->total_threads--;
    }
    
    spinlock_buka(&sched->lock);
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_tick
 * --------------
 * Handler tick timer untuk scheduler.
 * Dipanggil dari timer interrupt.
 */
void scheduler_tick(void)
{
    scheduler_t *sched;
    thread_t *current;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    sched->tick_count++;
    
    /* Update CPU usage */
    current = sched->current_thread;
    if (current == NULL || current == sched->idle_thread) {
        update_cpu_usage(cpu_id, BENAR);
    } else {
        update_cpu_usage(cpu_id, SALAH);
    }
    
    /* Cek untuk priority boost */
    if (sched->tick_count - sched->last_boost >= BOOST_INTERVAL) {
        boost_prioritas();
    }
    
    /* Cek untuk load balancing */
    if (sched->tick_count % LOAD_AVG_INTERVAL == 0) {
        hitung_load_avg();
        load_balance_check();
    }
    
    /* Tidak ada proses yang berjalan */
    if (current == NULL || current == sched->idle_thread) {
        sched->flags |= SCHED_FLAG_NEED_RESCHED;
        return;
    }
    
    /* Update CPU time */
    current->cpu_time++;
    if (current->proses != NULL) {
        current->proses->cpu_time++;
    }
    
    /* Kurangi quantum */
    if (current->quantum > 0) {
        current->quantum--;
    }
    
    /* Cek apakah quantum habis */
    if (current->quantum == 0) {
        sched->flags |= SCHED_FLAG_NEED_RESCHED;
        g_cpu_stats[cpu_id].timeouts++;
        g_sched_stats.total_timeouts++;
    }
}

/*
 * scheduler_schedule
 * ------------------
 * Jalankan scheduling.
 * Memilih thread berikutnya dan melakukan context switch.
 */
void scheduler_schedule(void)
{
    scheduler_t *sched;
    thread_t *prev;
    thread_t *next;
    tak_bertanda32_t cpu_id;
    tak_bertanda32_t idx;
    
    if (!g_scheduler_initialized) {
        return;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    spinlock_kunci(&sched->lock);
    
    /* Clear need_resched flag */
    sched->flags &= ~SCHED_FLAG_NEED_RESCHED;
    
    /* Ambil thread saat ini */
    prev = sched->current_thread;
    
    /* Handle thread sebelumnya */
    if (prev != NULL && prev != sched->idle_thread &&
        prev->status == THREAD_STATUS_JALAN) {
        
        /* Thread masih runnable, masukkan kembali ke antrian */
        /* Turunkan prioritas jika bukan realtime */
        if (prev->prioritas < PRIORITAS_REALTIME &&
            prev->prioritas > PRIORITAS_RENDAH) {
            prev->prioritas++;
        }
        
        /* Reset quantum */
        prev->quantum = dapatkan_quantum(prev->prioritas);
        prev->status = THREAD_STATUS_SIAP;
        
        /* Masukkan ke antrian */
        idx = dapatkan_runqueue_idx(prev->prioritas);
        if (idx >= JUMLAH_ANTRIAN) {
            idx = JUMLAH_ANTRIAN - 1;
        }
        
        runqueue_tambah(&sched->antrian[idx], prev);
    }
    
    /* Pilih thread berikutnya */
    next = pilih_thread_berikutnya(sched);
    
    /* Jika tidak ada, gunakan idle */
    if (next == NULL) {
        next = sched->idle_thread;
    }
    
    /* Set status */
    if (next != NULL) {
        next->status = THREAD_STATUS_JALAN;
        next->last_cpu = cpu_id;
        sched->current_thread = next;
    }
    
    /* Update counter */
    sched->context_switches++;
    g_cpu_stats[cpu_id].context_switches++;
    g_sched_stats.total_switches++;
    
    spinlock_buka(&sched->lock);
    
    /* Lakukan context switch jika berbeda */
    if (next != NULL && next != prev) {
        /* Update TSS untuk x86 */
#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_X86_64)
        if (next->kstack_ptr != NULL) {
            tss_set_kernel_stack((tak_bertanda64_t)(alamat_ptr_t)next->kstack_ptr);
        }
#endif
        
        context_switch_thread(prev, next);
    }
}

/*
 * scheduler_yield
 * ---------------
 * Relinquish CPU secara sukarela.
 *
 * Return: Status operasi
 */
status_t scheduler_yield(void)
{
    scheduler_t *sched;
    thread_t *current;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return STATUS_GAGAL;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    current = sched->current_thread;
    
    if (current == NULL || current == sched->idle_thread) {
        return STATUS_BERHASIL;
    }
    
    g_cpu_stats[cpu_id].yields++;
    g_sched_stats.total_yields++;
    
    /* Set flag untuk reschedule */
    sched->flags |= SCHED_FLAG_NEED_RESCHED;
    
    /* Jalankan schedule */
    scheduler_schedule();
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_preempt
 * -----------------
 * Force preemption.
 */
void scheduler_preempt(void)
{
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    sched->flags |= SCHED_FLAG_NEED_RESCHED;
    
    g_cpu_stats[cpu_id].preemptions++;
    g_sched_stats.total_preemptions++;
}

/*
 * scheduler_block
 * ---------------
 * Blok thread saat ini.
 *
 * Parameter:
 *   reason - Alasan pemblokiran
 *
 * Return: Status operasi
 */
status_t scheduler_block(tak_bertanda32_t reason)
{
    scheduler_t *sched;
    thread_t *current;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return STATUS_GAGAL;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    current = sched->current_thread;
    
    if (current == NULL) {
        return STATUS_GAGAL;
    }
    
    spinlock_kunci(&sched->lock);
    
    current->status = THREAD_STATUS_TUNGGU;
    current->block_reason = reason;
    
    if (sched->total_threads > 0) {
        sched->total_threads--;
    }
    
    g_cpu_stats[cpu_id].sleeps++;
    
    spinlock_buka(&sched->lock);
    
    /* Schedule ke thread lain */
    scheduler_schedule();
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_block_timeout
 * -----------------------
 * Blok thread dengan timeout.
 *
 * Parameter:
 *   reason  - Alasan pemblokiran
 *   timeout - Timeout dalam ticks
 *
 * Return: Status operasi
 */
status_t scheduler_block_timeout(tak_bertanda32_t reason,
                                 tak_bertanda64_t timeout)
{
    scheduler_t *sched;
    thread_t *current;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return STATUS_GAGAL;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    current = sched->current_thread;
    
    if (current == NULL) {
        return STATUS_GAGAL;
    }
    
    spinlock_kunci(&sched->lock);
    
    current->status = THREAD_STATUS_TUNGGU;
    current->block_reason = reason;
    current->wake_time = hal_timer_get_ticks() + timeout;
    
    if (sched->total_threads > 0) {
        sched->total_threads--;
    }
    
    g_cpu_stats[cpu_id].sleeps++;
    
    spinlock_buka(&sched->lock);
    
    /* Schedule ke thread lain */
    scheduler_schedule();
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_unblock
 * -----------------
 * Buka blokiran thread.
 *
 * Parameter:
 *   thread - Pointer ke thread
 *
 * Return: Status operasi
 */
status_t scheduler_unblock(thread_t *thread)
{
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    tak_bertanda32_t idx;
    
    if (!g_scheduler_initialized || thread == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    cpu_id = thread->last_cpu;
    if (cpu_id >= CPU_MAKS) {
        cpu_id = 0;
    }
    
    sched = &g_sched[cpu_id];
    
    spinlock_kunci(&sched->lock);
    
    if (thread->status != THREAD_STATUS_TUNGGU &&
        thread->status != THREAD_STATUS_SLEEP) {
        spinlock_buka(&sched->lock);
        return STATUS_BERHASIL;
    }
    
    /* Set status dan quantum */
    thread->status = THREAD_STATUS_SIAP;
    thread->quantum = dapatkan_quantum(thread->prioritas);
    thread->wake_time = 0;
    thread->block_reason = BLOCK_ALASAN_NONE;
    
    /* Masukkan ke antrian */
    idx = dapatkan_runqueue_idx(thread->prioritas);
    if (idx >= JUMLAH_ANTRIAN) {
        idx = JUMLAH_ANTRIAN - 1;
    }
    
    runqueue_tambah(&sched->antrian[idx], thread);
    
    sched->total_threads++;
    
    g_cpu_stats[cpu_id].wakeups++;
    
    spinlock_buka(&sched->lock);
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_set_prioritas
 * -----------------------
 * Set prioritas thread.
 *
 * Parameter:
 *   thread    - Pointer ke thread
 *   prioritas - Prioritas baru
 *
 * Return: Status operasi
 */
status_t scheduler_set_prioritas(thread_t *thread, tak_bertanda32_t prioritas)
{
    tak_bertanda32_t old_idx;
    tak_bertanda32_t new_idx;
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized || thread == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    if (prioritas > PRIORITAS_KRITIS) {
        return STATUS_PARAM_INVALID;
    }
    
    cpu_id = thread->last_cpu;
    if (cpu_id >= CPU_MAKS) {
        cpu_id = 0;
    }
    
    sched = &g_sched[cpu_id];
    
    spinlock_kunci(&sched->lock);
    
    /* Jika thread dalam antrian, pindahkan */
    if (thread->status == THREAD_STATUS_SIAP) {
        old_idx = dapatkan_runqueue_idx(thread->prioritas);
        new_idx = dapatkan_runqueue_idx(prioritas);
        
        if (old_idx < JUMLAH_ANTRIAN && new_idx < JUMLAH_ANTRIAN &&
            old_idx != new_idx) {
            
            runqueue_hapus(&sched->antrian[old_idx], thread);
            runqueue_tambah(&sched->antrian[new_idx], thread);
        }
    }
    
    /* Update prioritas */
    thread->prioritas = prioritas;
    thread->quantum = dapatkan_quantum(prioritas);
    
    spinlock_buka(&sched->lock);
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_boost_prioritas
 * -------------------------
 * Boost prioritas thread.
 *
 * Parameter:
 *   thread - Pointer ke thread
 *
 * Return: Status operasi
 */
status_t scheduler_boost_prioritas(thread_t *thread)
{
    if (thread == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Naikkan prioritas satu level */
    if (thread->prioritas > 0) {
        return scheduler_set_prioritas(thread, thread->prioritas - 1);
    }
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_dapat_beban
 * ---------------------
 * Dapatkan beban scheduler.
 *
 * Return: Jumlah thread runnable
 */
tak_bertanda32_t scheduler_dapat_beban(void)
{
    tak_bertanda32_t total;
    tak_bertanda32_t i;
    
    total = 0;
    
    for (i = 0; i < hal_cpu_get_count() && i < CPU_MAKS; i++) {
        total += g_sched[i].total_threads;
    }
    
    return total;
}

/*
 * scheduler_dapat_load_avg
 * ------------------------
 * Dapatkan load average.
 *
 * Parameter:
 *   avg1 - Pointer untuk 1 minute average
 *   avg5 - Pointer untuk 5 minute average
 *   avg15 - Pointer untuk 15 minute average
 */
void scheduler_dapat_load_avg(tak_bertanda32_t *avg1,
                              tak_bertanda32_t *avg5,
                              tak_bertanda32_t *avg15)
{
    if (avg1 != NULL) {
        *avg1 = g_load_avg[0];
    }
    
    if (avg5 != NULL) {
        *avg5 = g_load_avg[1];
    }
    
    if (avg15 != NULL) {
        *avg15 = g_load_avg[2];
    }
}

/*
 * scheduler_perlu_resched
 * -----------------------
 * Cek apakah perlu reschedule.
 *
 * Return: BENAR jika perlu, SALAH jika tidak
 */
bool_t scheduler_perlu_resched(void)
{
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return SALAH;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    return (sched->flags & SCHED_FLAG_NEED_RESCHED) ? BENAR : SALAH;
}

/*
 * scheduler_set_need_resched
 * --------------------------
 * Set flag need_resched.
 */
void scheduler_set_need_resched(void)
{
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    sched->flags |= SCHED_FLAG_NEED_RESCHED;
}

/*
 * scheduler_clear_need_resched
 * ----------------------------
 * Clear flag need_resched.
 */
void scheduler_clear_need_resched(void)
{
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    sched->flags &= ~SCHED_FLAG_NEED_RESCHED;
}

/*
 * scheduler_dapat_current
 * -----------------------
 * Dapatkan thread saat ini.
 *
 * Return: Pointer ke thread
 */
thread_t *scheduler_dapat_current(void)
{
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return NULL;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    return sched->current_thread;
}

/*
 * scheduler_set_current
 * ---------------------
 * Set thread saat ini.
 *
 * Parameter:
 *   thread - Pointer ke thread
 */
void scheduler_set_current(thread_t *thread)
{
    scheduler_t *sched;
    tak_bertanda32_t cpu_id;
    
    if (!g_scheduler_initialized) {
        return;
    }
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    sched->current_thread = thread;
}

/*
 * scheduler_set_affinity
 * ----------------------
 * Set CPU affinity untuk thread.
 *
 * Parameter:
 *   thread - Pointer ke thread
 *   mask   - CPU affinity mask
 *
 * Return: Status operasi
 */
status_t scheduler_set_affinity(thread_t *thread, tak_bertanda32_t mask)
{
    tak_bertanda32_t cpu_count;
    
    if (!g_scheduler_initialized || thread == NULL) {
        return STATUS_PARAM_INVALID;
    }
    
    cpu_count = hal_cpu_get_count();
    
    /* Pastikan mask valid */
    if (mask == 0) {
        mask = 0xFFFFFFFF;
    }
    
    /* Batasi ke CPU yang tersedia */
    if (cpu_count < 32) {
        mask &= ((1UL << cpu_count) - 1);
    }
    
    thread->cpu_affinity = mask;
    
    return STATUS_BERHASIL;
}

/*
 * scheduler_dapat_affinity
 * ------------------------
 * Dapatkan CPU affinity thread.
 *
 * Parameter:
 *   thread - Pointer ke thread
 *
 * Return: CPU affinity mask
 */
tak_bertanda32_t scheduler_dapat_affinity(thread_t *thread)
{
    if (thread == NULL) {
        return 0;
    }
    
    return thread->cpu_affinity;
}

/*
 * scheduler_dapat_statistik
 * -------------------------
 * Dapatkan statistik scheduler.
 *
 * Parameter:
 *   switches    - Pointer untuk jumlah context switch
 *   preemptions - Pointer untuk jumlah preemption
 *   yields      - Pointer untuk jumlah yield
 *   timeouts    - Pointer untuk jumlah timeout
 */
void scheduler_dapat_statistik(tak_bertanda64_t *switches,
                               tak_bertanda64_t *preemptions,
                               tak_bertanda64_t *yields,
                               tak_bertanda64_t *timeouts)
{
    if (switches != NULL) {
        *switches = g_sched_stats.total_switches;
    }
    
    if (preemptions != NULL) {
        *preemptions = g_sched_stats.total_preemptions;
    }
    
    if (yields != NULL) {
        *yields = g_sched_stats.total_yields;
    }
    
    if (timeouts != NULL) {
        *timeouts = g_sched_stats.total_timeouts;
    }
}

/*
 * scheduler_print_stats
 * ---------------------
 * Print statistik scheduler.
 */
void scheduler_print_stats(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t cpu_count;
    
    kernel_printf("\n[SCHED] Statistik Scheduler:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Context Switches : %lu\n",
                  g_sched_stats.total_switches);
    kernel_printf("  Preemptions      : %lu\n",
                  g_sched_stats.total_preemptions);
    kernel_printf("  Yields           : %lu\n",
                  g_sched_stats.total_yields);
    kernel_printf("  Timeouts         : %lu\n",
                  g_sched_stats.total_timeouts);
    kernel_printf("  Migrations       : %lu\n",
                  g_sched_stats.total_migrations);
    kernel_printf("  Priority Boosts  : %lu\n",
                  g_sched_stats.boost_count);
    kernel_printf("  Load Balances    : %lu\n",
                  g_sched_stats.load_balance_count);
    kernel_printf("========================================\n");
    kernel_printf("  Load Average: %lu.%02lu, %lu.%02lu, %lu.%02lu\n",
                  g_load_avg[0] >> 8,
                  (g_load_avg[0] & 0xFF) * 100 / 256,
                  g_load_avg[1] >> 8,
                  (g_load_avg[1] & 0xFF) * 100 / 256,
                  g_load_avg[2] >> 8,
                  (g_load_avg[2] & 0xFF) * 100 / 256);
    
    cpu_count = hal_cpu_get_count();
    
    kernel_printf("\n  Per-CPU Statistics:\n");
    for (i = 0; i < cpu_count && i < CPU_MAKS; i++) {
        kernel_printf("    CPU %lu: Load %lu%%, Switches %lu\n",
                      (tak_bertanda64_t)i,
                      (tak_bertanda64_t)g_cpu_usage[i].load,
                      g_cpu_stats[i].context_switches);
    }
    
    kernel_printf("========================================\n");
}

/*
 * scheduler_print_queues
 * ----------------------
 * Print status antrian scheduler.
 */
void scheduler_print_queues(void)
{
    scheduler_t *sched;
    runqueue_t *rq;
    thread_t *thread;
    tak_bertanda32_t cpu_id;
    tak_bertanda32_t i;
    const char *prio_str[] = {
        "Kritis", "Realtime", "Tinggi", "Normal",
        "Rendah", "Idle", "Batch", "Other"
    };
    
    cpu_id = dapatkan_cpu_id();
    sched = &g_sched[cpu_id];
    
    kernel_printf("\n[SCHED] Status Antrian CPU %lu:\n",
                  (tak_bertanda64_t)cpu_id);
    kernel_printf("========================================\n");
    
    /* Print realtime queue */
    kernel_printf("  Realtime Queue (%lu): ", sched->rt_queue.count);
    thread = sched->rt_queue.head;
    while (thread != NULL) {
        kernel_printf("[%lu:%s] ",
                      (tak_bertanda64_t)thread->tid,
                      thread->proses != NULL ? thread->proses->nama : "?");
        thread = thread->next;
    }
    kernel_printf("\n");
    
    /* Print antrian normal */
    for (i = 0; i < JUMLAH_ANTRIAN; i++) {
        rq = &sched->antrian[i];
        
        kernel_printf("  Antrian %s (%lu): ",
                      prio_str[i < 8 ? i : 7],
                      (tak_bertanda64_t)rq->count);
        
        thread = rq->head;
        while (thread != NULL) {
            kernel_printf("[%lu] ", (tak_bertanda64_t)thread->tid);
            thread = thread->next;
        }
        
        kernel_printf("\n");
    }
    
    kernel_printf("========================================\n");
    kernel_printf("  Total Runnable: %lu threads\n",
                  (tak_bertanda64_t)sched->total_threads);
}

/*
 * scheduler_print_load
 * --------------------
 * Print load average.
 */
void scheduler_print_load(void)
{
    kernel_printf("\n[SCHED] Load Average:\n");
    kernel_printf("========================================\n");
    kernel_printf("  1 min : %lu.%02lu\n",
                  g_load_avg[0] >> 8,
                  (g_load_avg[0] & 0xFF) * 100 / 256);
    kernel_printf("  5 min : %lu.%02lu\n",
                  g_load_avg[1] >> 8,
                  (g_load_avg[1] & 0xFF) * 100 / 256);
    kernel_printf("  15 min: %lu.%02lu\n",
                  g_load_avg[2] >> 8,
                  (g_load_avg[2] & 0xFF) * 100 / 256);
    kernel_printf("========================================\n");
}
