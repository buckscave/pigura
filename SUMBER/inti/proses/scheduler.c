/*
 * PIGURA OS - SCHEDULER.C
 * ------------------------
 * Implementasi penjadwalan proses dengan algoritma prioritas.
 *
 * Berkas ini berisi implementasi scheduler preemptive dengan dukungan
 * multi-level feedback queue dan priority scheduling.
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

/* Jumlah antrian prioritas */
#define JUMLAH_ANTRIAN         4

/* Quantum untuk setiap level prioritas */
#define QUANTUM_REALTIME       20
#define QUANTUM_TINGGI         15
#define QUANTUM_NORMAL         10
#define QUANTUM_RENDAH         5

/* Faktor bonus untuk nice value */
#define NICE_DEFAULT           0
#define NICE_MIN               -20
#define NICE_MAX               19

/* Threshold untuk boost priority */
#define BOOST_THRESHOLD        100

/* Flag scheduler */
#define SCHED_FLAG_PREEMPT     0x01
#define SCHED_FLAG_NEED_RESCHED 0x02

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur antrian runqueue */
typedef struct {
    proses_t *head;                 /* Proses pertama dalam antrian */
    proses_t *tail;                 /* Proses terakhir dalam antrian */
    tak_bertanda32_t count;         /* Jumlah proses dalam antrian */
} runqueue_t;

/* Struktur scheduler */
typedef struct {
    runqueue_t antrian[JUMLAH_ANTRIAN]; /* Antrian per prioritas */
    tak_bertanda32_t total_proses;      /* Total proses runnable */
    tak_bertanda32_t tick_count;        /* Counter tick */
    tak_bertanda32_t context_switches;  /* Jumlah context switch */
    tak_bertanda32_t last_boost;        /* Tick boost terakhir */
    spinlock_t lock;                    /* Lock untuk thread safety */
    tak_bertanda32_t flags;             /* Flag scheduler */
    proses_t *idle_proses;              /* Pointer ke idle proses */
} scheduler_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* State scheduler */
static scheduler_t sched = {0};

/* Status inisialisasi */
static bool_t scheduler_initialized = SALAH;

/* Statistik scheduler */
static struct {
    tak_bertanda64_t switches;
    tak_bertanda64_t preemptions;
    tak_bertanda64_t yields;
    tak_bertanda64_t timeouts;
} sched_stats = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
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
}

/*
 * runqueue_tambah
 * ---------------
 * Tambahkan proses ke akhir runqueue.
 *
 * Parameter:
 *   rq     - Pointer ke runqueue
 *   proses - Pointer ke proses
 */
static void runqueue_tambah(runqueue_t *rq, proses_t *proses)
{
    if (rq == NULL || proses == NULL) {
        return;
    }

    proses->next = NULL;

    if (rq->tail == NULL) {
        rq->head = proses;
        rq->tail = proses;
        proses->prev = NULL;
    } else {
        rq->tail->next = proses;
        proses->prev = rq->tail;
        rq->tail = proses;
    }

    rq->count++;
}

/*
 * runqueue_hapus
 * --------------
 * Hapus proses dari runqueue.
 *
 * Parameter:
 *   rq     - Pointer ke runqueue
 *   proses - Pointer ke proses
 */
static void runqueue_hapus(runqueue_t *rq, proses_t *proses)
{
    if (rq == NULL || proses == NULL) {
        return;
    }

    if (proses->prev != NULL) {
        proses->prev->next = proses->next;
    } else {
        rq->head = proses->next;
    }

    if (proses->next != NULL) {
        proses->next->prev = proses->prev;
    } else {
        rq->tail = proses->prev;
    }

    proses->next = NULL;
    proses->prev = NULL;
    rq->count--;
}

/*
 * runqueue_ambil
 * --------------
 * Ambil proses pertama dari runqueue.
 *
 * Parameter:
 *   rq - Pointer ke runqueue
 *
 * Return: Pointer ke proses, atau NULL jika kosong
 */
static proses_t *runqueue_ambil(runqueue_t *rq)
{
    proses_t *proses;

    if (rq == NULL || rq->head == NULL) {
        return NULL;
    }

    proses = rq->head;
    runqueue_hapus(rq, proses);

    return proses;
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
static tak_bertanda32_t dapatkan_quantum(prioritas_t prioritas)
{
    switch (prioritas) {
        case PRIORITAS_REALTIME:
            return QUANTUM_REALTIME;
        case PRIORITAS_TINGGI:
            return QUANTUM_TINGGI;
        case PRIORITAS_NORMAL:
            return QUANTUM_NORMAL;
        case PRIORITAS_RENDAH:
        default:
            return QUANTUM_RENDAH;
    }
}

/*
 * dapatkan_antrian_idx
 * --------------------
 * Dapatkan indeks antrian berdasarkan prioritas.
 *
 * Parameter:
 *   prioritas - Level prioritas
 *
 * Return: Indeks antrian (0-3)
 */
static tak_bertanda32_t dapatkan_antrian_idx(prioritas_t prioritas)
{
    switch (prioritas) {
        case PRIORITAS_REALTIME:
            return 0;
        case PRIORITAS_TINGGI:
            return 1;
        case PRIORITAS_NORMAL:
            return 2;
        case PRIORITAS_RENDAH:
        default:
            return 3;
    }
}

/*
 * boost_prioritas
 * ---------------
 * Boost prioritas proses yang kelaparan.
 */
static void boost_prioritas(void)
{
    tak_bertanda32_t i;
    proses_t *proses;
    proses_t *next;

    /* Pindahkan semua proses ke antrian normal */
    for (i = 1; i < JUMLAH_ANTRIAN; i++) {
        proses = sched.antrian[i].head;

        while (proses != NULL) {
            next = proses->next;

            runqueue_hapus(&sched.antrian[i], proses);
            proses->prioritas = PRIORITAS_NORMAL;
            proses->quantum = QUANTUM_NORMAL;
            runqueue_tambah(&sched.antrian[2], proses);

            proses = next;
        }
    }

    sched.last_boost = sched.tick_count;
}

/*
 * cek_boost
 * ---------
 * Cek apakah perlu boost prioritas.
 */
static void cek_boost(void)
{
    if (sched.tick_count - sched.last_boost >= BOOST_THRESHOLD) {
        boost_prioritas();
    }
}

/*
 * pilih_proses_berikutnya
 * -----------------------
 * Pilih proses berikutnya untuk dijalankan.
 *
 * Return: Pointer ke proses, atau NULL
 */
static proses_t *pilih_proses_berikutnya(void)
{
    tak_bertanda32_t i;
    proses_t *proses;

    /* Cari dari antrian prioritas tertinggi */
    for (i = 0; i < JUMLAH_ANTRIAN; i++) {
        if (sched.antrian[i].count > 0) {
            proses = runqueue_ambil(&sched.antrian[i]);
            return proses;
        }
    }

    /* Tidak ada proses runnable, kembalikan idle */
    return sched.idle_proses;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
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

    if (scheduler_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Reset state */
    kernel_memset(&sched, 0, sizeof(sched));

    /* Inisialisasi antrian */
    for (i = 0; i < JUMLAH_ANTRIAN; i++) {
        runqueue_init(&sched.antrian[i]);
    }

    /* Init lock */
    spinlock_init(&sched.lock);

    /* Reset stats */
    kernel_memset(&sched_stats, 0, sizeof(sched_stats));

    /* Set idle proses */
    sched.idle_proses = proses_get_idle();

    scheduler_initialized = BENAR;

    kernel_printf("[SCHED] Scheduler initialized\n");
    kernel_printf("        Quantum: RT=%d, HI=%d, NM=%d, LO=%d\n",
                  QUANTUM_REALTIME, QUANTUM_TINGGI,
                  QUANTUM_NORMAL, QUANTUM_RENDAH);

    return STATUS_BERHASIL;
}

/*
 * scheduler_tambah_proses
 * -----------------------
 * Tambahkan proses ke scheduler.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: Status operasi
 */
status_t scheduler_tambah_proses(proses_t *proses)
{
    tak_bertanda32_t idx;

    if (!scheduler_initialized || proses == NULL) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&sched.lock);

    /* Dapatkan indeks antrian */
    idx = dapatkan_antrian_idx(proses->prioritas);

    /* Set quantum */
    proses->quantum = dapatkan_quantum(proses->prioritas);

    /* Set status */
    proses->status = PROSES_STATUS_SIAP;

    /* Tambahkan ke antrian */
    runqueue_tambah(&sched.antrian[idx], proses);
    sched.total_proses++;

    spinlock_buka(&sched.lock);

    return STATUS_BERHASIL;
}

/*
 * scheduler_hapus_proses
 * ----------------------
 * Hapus proses dari scheduler.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: Status operasi
 */
status_t scheduler_hapus_proses(proses_t *proses)
{
    tak_bertanda32_t idx;

    if (!scheduler_initialized || proses == NULL) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&sched.lock);

    /* Dapatkan indeks antrian */
    idx = dapatkan_antrian_idx(proses->prioritas);

    /* Hapus dari antrian */
    runqueue_hapus(&sched.antrian[idx], proses);

    if (sched.total_proses > 0) {
        sched.total_proses--;
    }

    spinlock_buka(&sched.lock);

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
    proses_t *current;

    if (!scheduler_initialized) {
        return;
    }

    sched.tick_count++;

    /* Cek boost */
    cek_boost();

    /* Cek proses saat ini */
    current = proses_get_current();

    if (current == NULL || current == sched.idle_proses) {
        sched.flags |= SCHED_FLAG_NEED_RESCHED;
        return;
    }

    /* Kurangi quantum */
    if (current->quantum > 0) {
        current->quantum--;
    }

    /* Update CPU time */
    current->cpu_time++;

    /* Cek apakah quantum habis */
    if (current->quantum == 0) {
        sched.flags |= SCHED_FLAG_NEED_RESCHED;
        sched_stats.timeouts++;
    }
}

/*
 * scheduler_schedule
 * ------------------
 * Jalankan scheduling.
 * Memilih proses berikutnya dan melakukan context switch.
 */
void scheduler_schedule(void)
{
    proses_t *prev;
    proses_t *next;
    tak_bertanda32_t idx;

    if (!scheduler_initialized) {
        return;
    }

    spinlock_kunci(&sched.lock);

    /* Ambil proses saat ini */
    prev = proses_get_current();

    /* Clear need_resched flag */
    sched.flags &= ~SCHED_FLAG_NEED_RESCHED;

    /* Handle proses sebelumnya */
    if (prev != NULL && prev != sched.idle_proses &&
        prev->status == PROSES_STATUS_JALAN) {

        /* Reset quantum */
        prev->quantum = dapatkan_quantum(prev->prioritas);

        /* Turunkan prioritas jika bukan realtime */
        if (prev->prioritas > PRIORITAS_REALTIME &&
            prev->prioritas < PRIORITAS_RENDAH) {
            prev->prioritas = (prioritas_t)(prev->prioritas + 1);
        }

        /* Kembalikan ke antrian */
        idx = dapatkan_antrian_idx(prev->prioritas);
        prev->status = PROSES_STATUS_SIAP;
        runqueue_tambah(&sched.antrian[idx], prev);
    }

    /* Pilih proses berikutnya */
    next = pilih_proses_berikutnya();

    if (next == NULL) {
        next = sched.idle_proses;
    }

    /* Set status */
    next->status = PROSES_STATUS_JALAN;

    /* Update counter */
    sched.context_switches++;
    sched_stats.switches++;

    spinlock_buka(&sched.lock);

    /* Lakukan context switch jika berbeda */
    if (next != prev) {
        context_switch(prev, next);
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
    proses_t *current;

    if (!scheduler_initialized) {
        return STATUS_GAGAL;
    }

    current = proses_get_current();

    if (current == NULL || current == sched.idle_proses) {
        return STATUS_BERHASIL;
    }

    sched_stats.yields++;
    sched.flags |= SCHED_FLAG_NEED_RESCHED;

    /* Jalankan schedule */
    scheduler_schedule();

    return STATUS_BERHASIL;
}

/*
 * scheduler_block
 * ---------------
 * Blok proses saat ini.
 *
 * Parameter:
 *   reason - Alasan pemblokiran
 *
 * Return: Status operasi
 */
status_t scheduler_block(tak_bertanda32_t reason)
{
    proses_t *current;

    if (!scheduler_initialized) {
        return STATUS_GAGAL;
    }

    current = proses_get_current();

    if (current == NULL) {
        return STATUS_GAGAL;
    }

    spinlock_kunci(&sched.lock);

    current->status = PROSES_STATUS_TUNGGU;
    sched.total_proses--;

    spinlock_buka(&sched.lock);

    /* Schedule ke proses lain */
    scheduler_schedule();

    return STATUS_BERHASIL;
}

/*
 * scheduler_unblock
 * -----------------
 * Buka blokiran proses.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: Status operasi
 */
status_t scheduler_unblock(proses_t *proses)
{
    tak_bertanda32_t idx;

    if (!scheduler_initialized || proses == NULL) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&sched.lock);

    if (proses->status != PROSES_STATUS_TUNGGU) {
        spinlock_buka(&sched.lock);
        return STATUS_BERHASIL;
    }

    /* Set status dan quantum */
    proses->status = PROSES_STATUS_SIAP;
    proses->quantum = dapatkan_quantum(proses->prioritas);

    /* Tambahkan ke antrian */
    idx = dapatkan_antrian_idx(proses->prioritas);
    runqueue_tambah(&sched.antrian[idx], proses);
    sched.total_proses++;

    spinlock_buka(&sched.lock);

    return STATUS_BERHASIL;
}

/*
 * scheduler_set_priority
 * ----------------------
 * Set prioritas proses.
 *
 * Parameter:
 *   proses    - Pointer ke proses
 *   prioritas - Prioritas baru
 *
 * Return: Status operasi
 */
status_t scheduler_set_priority(proses_t *proses, prioritas_t prioritas)
{
    tak_bertanda32_t old_idx;
    tak_bertanda32_t new_idx;

    if (!scheduler_initialized || proses == NULL) {
        return STATUS_PARAM_INVALID;
    }

    if (prioritas > PRIORITAS_REALTIME) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&sched.lock);

    /* Jika proses dalam antrian, pindahkan */
    if (proses->status == PROSES_STATUS_SIAP) {
        old_idx = dapatkan_antrian_idx(proses->prioritas);
        new_idx = dapatkan_antrian_idx(prioritas);

        if (old_idx != new_idx) {
            runqueue_hapus(&sched.antrian[old_idx], proses);
            runqueue_tambah(&sched.antrian[new_idx], proses);
        }
    }

    /* Update prioritas */
    proses->prioritas = prioritas;
    proses->quantum = dapatkan_quantum(prioritas);

    spinlock_buka(&sched.lock);

    return STATUS_BERHASIL;
}

/*
 * scheduler_get_load
 * ------------------
 * Dapatkan beban scheduler.
 *
 * Return: Jumlah proses runnable
 */
tak_bertanda32_t scheduler_get_load(void)
{
    return sched.total_proses;
}

/*
 * scheduler_need_resched
 * ----------------------
 * Cek apakah perlu reschedule.
 *
 * Return: BENAR jika perlu, SALAH jika tidak
 */
bool_t scheduler_need_resched(void)
{
    return (sched.flags & SCHED_FLAG_NEED_RESCHED) ? BENAR : SALAH;
}

/*
 * scheduler_get_stats
 * -------------------
 * Dapatkan statistik scheduler.
 *
 * Parameter:
 *   switches    - Pointer untuk jumlah context switch
 *   preemptions - Pointer untuk jumlah preemption
 *   yields      - Pointer untuk jumlah yield
 *   timeouts    - Pointer untuk jumlah timeout
 */
void scheduler_get_stats(tak_bertanda64_t *switches,
                         tak_bertanda64_t *preemptions,
                         tak_bertanda64_t *yields,
                         tak_bertanda64_t *timeouts)
{
    if (switches != NULL) {
        *switches = sched_stats.switches;
    }

    if (preemptions != NULL) {
        *preemptions = sched_stats.preemptions;
    }

    if (yields != NULL) {
        *yields = sched_stats.yields;
    }

    if (timeouts != NULL) {
        *timeouts = sched_stats.timeouts;
    }
}

/*
 * scheduler_print_stats
 * ---------------------
 * Print statistik scheduler.
 */
void scheduler_print_stats(void)
{
    kernel_printf("\n[SCHED] Statistik Scheduler:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Context Switches : %lu\n", sched_stats.switches);
    kernel_printf("  Preemptions      : %lu\n", sched_stats.preemptions);
    kernel_printf("  Yields           : %lu\n", sched_stats.yields);
    kernel_printf("  Timeouts         : %lu\n", sched_stats.timeouts);
    kernel_printf("  Load Average     : %lu proses\n",
                  (tak_bertanda64_t)sched.total_proses);
    kernel_printf("  Tick Count       : %lu\n",
                  (tak_bertanda64_t)sched.tick_count);
    kernel_printf("========================================\n");
}

/*
 * scheduler_print_queues
 * ----------------------
 * Print status antrian scheduler.
 */
void scheduler_print_queues(void)
{
    tak_bertanda32_t i;
    proses_t *proses;
    const char *prio_str[] = {
        "REALTIME", "TINGGI", "NORMAL", "RENDAH"
    };

    kernel_printf("\n[SCHED] Status Antrian:\n");
    kernel_printf("========================================\n");

    for (i = 0; i < JUMLAH_ANTRIAN; i++) {
        kernel_printf("  Antrian %s (%u): ", prio_str[i],
                      sched.antrian[i].count);

        proses = sched.antrian[i].head;
        while (proses != NULL) {
            kernel_printf("[%lu:%s] ", (tak_bertanda64_t)proses->pid,
                          proses->nama);
            proses = proses->next;
        }

        kernel_printf("\n");
    }

    kernel_printf("========================================\n");
    kernel_printf("  Total Runnable: %lu proses\n",
                  (tak_bertanda64_t)sched.total_proses);
}
