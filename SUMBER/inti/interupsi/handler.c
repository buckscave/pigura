/*
 * PIGURA OS - HANDLER.C
 * ---------------------
 * Implementasi generic interrupt handler.
 *
 * Berkas ini berisi implementasi sistem penanganan interrupt
 * yang generik, mendukung shared interrupts dan chaining.
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

/* Maximum handlers per vector */
#define MAX_HANDLERS_PER_VECTOR     8

/* Handler flags */
#define HANDLER_FLAG_NONE           0x00
#define HANDLER_FLAG_ENABLED        0x01
#define HANDLER_FLAG_EXCLUSIVE      0x02
#define HANDLER_FLAG_SHARED         0x04
#define HANDLER_FLAG_FAST           0x08

/* Handler status */
#define HANDLER_STATUS_INACTIVE     0
#define HANDLER_STATUS_ACTIVE       1

/*
 * ============================================================================
 * STRUKTUR DATA LOKAL (LOCAL DATA STRUCTURES)
 * ============================================================================
 */

/* Interrupt handler entry */
typedef struct handler_entry {
    tak_bertanda32_t flags;                 /* Handler flags */
    tak_bertanda32_t status;                /* Current status */
    tak_bertanda32_t priority;              /* Handler priority */
    tak_bertanda32_t call_count;            /* Number of times called */
    tak_bertanda32_t error_count;           /* Error count */
    const char *name;                       /* Handler name */
    void *dev_id;                           /* Device ID for shared */
    int (*handler)(void *dev_id);           /* Handler function */
    struct handler_entry *next;             /* Next handler in chain */
} handler_entry_t;

/* Vector handler table */
typedef struct {
    handler_entry_t *handlers;              /* Handler chain */
    tak_bertanda32_t handler_count;         /* Number of handlers */
    tak_bertanda32_t flags;                 /* Vector flags */
    spinlock_t lock;                        /* Lock for this vector */
} vector_table_t;

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Vector handler table */
static vector_table_t vector_table[JUMLAH_ISR];

/* Handler pool for allocation */
#define HANDLER_POOL_SIZE       256
static handler_entry_t handler_pool[HANDLER_POOL_SIZE];
static handler_entry_t *handler_free_list = NULL;

/* Initialization flag */
static bool_t handler_initialized = SALAH;

/* Global statistics */
static struct {
    tak_bertanda64_t total_calls;
    tak_bertanda64_t total_errors;
    tak_bertanda64_t spurious_count;
} handler_stats = {0};

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * allocate_handler_entry
 * ----------------------
 * Alokasikan entry handler dari pool.
 *
 * Return: Pointer ke entry, atau NULL
 */
static handler_entry_t *allocate_handler_entry(void)
{
    handler_entry_t *entry;

    if (handler_free_list == NULL) {
        return NULL;
    }

    entry = handler_free_list;
    handler_free_list = entry->next;

    /* Clear entry */
    kernel_memset(entry, 0, sizeof(handler_entry_t));

    return entry;
}

/*
 * free_handler_entry
 * ------------------
 * Kembalikan entry handler ke pool.
 *
 * Parameter:
 *   entry - Pointer ke entry
 */
static void free_handler_entry(handler_entry_t *entry)
{
    if (entry == NULL) {
        return;
    }

    entry->next = handler_free_list;
    handler_free_list = entry;
}

/*
 * insert_handler_sorted
 * ---------------------
 * Insert handler ke chain berdasarkan prioritas.
 *
 * Parameter:
 *   vector - Nomor vector
 *   entry  - Entry handler
 */
static void insert_handler_sorted(tak_bertanda32_t vector,
                                  handler_entry_t *entry)
{
    handler_entry_t **pp;
    handler_entry_t *current;

    pp = &vector_table[vector].handlers;
    current = *pp;

    /* Cari posisi yang tepat berdasarkan prioritas */
    while (current != NULL && current->priority >= entry->priority) {
        pp = &current->next;
        current = *pp;
    }

    entry->next = current;
    *pp = entry;

    vector_table[vector].handler_count++;
}

/*
 * remove_handler_entry
 * --------------------
 * Hapus handler dari chain.
 *
 * Parameter:
 *   vector - Nomor vector
 *   entry  - Entry handler
 *
 * Return: Status operasi
 */
static status_t remove_handler_entry(tak_bertanda32_t vector,
                                     handler_entry_t *entry)
{
    handler_entry_t **pp;
    handler_entry_t *current;

    pp = &vector_table[vector].handlers;
    current = *pp;

    while (current != NULL) {
        if (current == entry) {
            *pp = current->next;
            vector_table[vector].handler_count--;
            return STATUS_BERHASIL;
        }
        pp = &current->next;
        current = *pp;
    }

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * call_handler_chain
 * ------------------
 * Panggil semua handler dalam chain.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return: Status (IRQ_HANDLED, IRQ_NONE)
 */
static int call_handler_chain(tak_bertanda32_t vector)
{
    handler_entry_t *entry;
    int handled = 0;

    entry = vector_table[vector].handlers;

    while (entry != NULL) {
        if (entry->flags & HANDLER_FLAG_ENABLED) {
            int result;

            entry->status = HANDLER_STATUS_ACTIVE;

            /* Panggil handler */
            result = entry->handler(entry->dev_id);

            entry->status = HANDLER_STATUS_INACTIVE;
            entry->call_count++;
            handler_stats.total_calls++;

            if (result < 0) {
                entry->error_count++;
                handler_stats.total_errors++;
            } else if (result > 0) {
                handled = 1;
            }
        }

        entry = entry->next;
    }

    return handled;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * handler_init
 * ------------
 * Inisialisasi sistem handler interrupt.
 *
 * Return: Status operasi
 */
status_t handler_init(void)
{
    tak_bertanda32_t i;

    if (handler_initialized) {
        return STATUS_SUDAH_ADA;
    }

    /* Initialize handler pool */
    for (i = 0; i < HANDLER_POOL_SIZE; i++) {
        handler_pool[i].next = handler_free_list;
        handler_free_list = &handler_pool[i];
    }

    /* Initialize vector table */
    for (i = 0; i < JUMLAH_ISR; i++) {
        vector_table[i].handlers = NULL;
        vector_table[i].handler_count = 0;
        vector_table[i].flags = 0;
        spinlock_init(&vector_table[i].lock);
    }

    /* Reset statistics */
    handler_stats.total_calls = 0;
    handler_stats.total_errors = 0;
    handler_stats.spurious_count = 0;

    handler_initialized = BENAR;

    kernel_printf("[HANDLER] Interrupt handler system initialized\n");

    return STATUS_BERHASIL;
}

/*
 * handler_register
 * ----------------
 * Register interrupt handler.
 *
 * Parameter:
 *   vector   - Nomor vector interrupt
 *   name     - Nama handler
 *   handler  - Fungsi handler
 *   dev_id   - Device ID (untuk shared interrupt)
 *   flags    - Handler flags
 *
 * Return: Status operasi
 */
status_t handler_register(tak_bertanda32_t vector, const char *name,
                          int (*handler)(void *), void *dev_id,
                          tak_bertanda32_t flags)
{
    handler_entry_t *entry;
    tak_bertanda32_t priority;

    if (!handler_initialized) {
        return STATUS_GAGAL;
    }

    if (vector >= JUMLAH_ISR || handler == NULL) {
        return STATUS_PARAM_INVALID;
    }

    /* Cek apakah bisa menerima handler baru */
    if (vector_table[vector].handler_count >= MAX_HANDLERS_PER_VECTOR) {
        return STATUS_BUSY;
    }

    /* Jika ada handler exclusive, tidak bisa menambah */
    if ((vector_table[vector].flags & HANDLER_FLAG_EXCLUSIVE) &&
        vector_table[vector].handler_count > 0) {
        return STATUS_BUSY;
    }

    spinlock_kunci(&vector_table[vector].lock);

    /* Allocate entry */
    entry = allocate_handler_entry();
    if (entry == NULL) {
        spinlock_buka(&vector_table[vector].lock);
        return STATUS_MEMORI_HABIS;
    }

    /* Set priority (lower number = higher priority) */
    priority = (flags >> 16) & 0xFF;
    if (priority == 0) {
        priority = 128;  /* Default priority */
    }

    /* Setup entry */
    entry->flags = flags | HANDLER_FLAG_ENABLED;
    entry->status = HANDLER_STATUS_INACTIVE;
    entry->priority = priority;
    entry->call_count = 0;
    entry->error_count = 0;
    entry->name = name;
    entry->dev_id = dev_id;
    entry->handler = handler;
    entry->next = NULL;

    /* Insert ke chain */
    insert_handler_sorted(vector, entry);

    /* Set exclusive flag jika perlu */
    if (flags & HANDLER_FLAG_EXCLUSIVE) {
        vector_table[vector].flags |= HANDLER_FLAG_EXCLUSIVE;
    }

    spinlock_buka(&vector_table[vector].lock);

    return STATUS_BERHASIL;
}

/*
 * handler_unregister
 * ------------------
 * Unregister interrupt handler.
 *
 * Parameter:
 *   vector  - Nomor vector interrupt
 *   handler - Fungsi handler
 *   dev_id  - Device ID
 *
 * Return: Status operasi
 */
status_t handler_unregister(tak_bertanda32_t vector,
                            int (*handler)(void *), void *dev_id)
{
    handler_entry_t *entry;
    status_t status;

    if (!handler_initialized) {
        return STATUS_GAGAL;
    }

    if (vector >= JUMLAH_ISR) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&vector_table[vector].lock);

    /* Cari entry */
    entry = vector_table[vector].handlers;

    while (entry != NULL) {
        if (entry->handler == handler && entry->dev_id == dev_id) {
            break;
        }
        entry = entry->next;
    }

    if (entry == NULL) {
        spinlock_buka(&vector_table[vector].lock);
        return STATUS_TIDAK_DITEMUKAN;
    }

    /* Remove dari chain */
    status = remove_handler_entry(vector, entry);

    if (status == STATUS_BERHASIL) {
        /* Clear exclusive flag jika tidak ada handler lagi */
        if (vector_table[vector].handler_count == 0) {
            vector_table[vector].flags = 0;
        }

        free_handler_entry(entry);
    }

    spinlock_buka(&vector_table[vector].lock);

    return status;
}

/*
 * handler_enable
 * --------------
 * Aktifkan handler.
 *
 * Parameter:
 *   vector - Nomor vector
 *   dev_id - Device ID
 *
 * Return: Status operasi
 */
status_t handler_enable(tak_bertanda32_t vector, void *dev_id)
{
    handler_entry_t *entry;

    if (!handler_initialized || vector >= JUMLAH_ISR) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&vector_table[vector].lock);

    entry = vector_table[vector].handlers;

    while (entry != NULL) {
        if (entry->dev_id == dev_id) {
            entry->flags |= HANDLER_FLAG_ENABLED;
            spinlock_buka(&vector_table[vector].lock);
            return STATUS_BERHASIL;
        }
        entry = entry->next;
    }

    spinlock_buka(&vector_table[vector].lock);

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * handler_disable
 * ---------------
 * Nonaktifkan handler.
 *
 * Parameter:
 *   vector - Nomor vector
 *   dev_id - Device ID
 *
 * Return: Status operasi
 */
status_t handler_disable(tak_bertanda32_t vector, void *dev_id)
{
    handler_entry_t *entry;

    if (!handler_initialized || vector >= JUMLAH_ISR) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&vector_table[vector].lock);

    entry = vector_table[vector].handlers;

    while (entry != NULL) {
        if (entry->dev_id == dev_id) {
            entry->flags &= ~HANDLER_FLAG_ENABLED;
            spinlock_buka(&vector_table[vector].lock);
            return STATUS_BERHASIL;
        }
        entry = entry->next;
    }

    spinlock_buka(&vector_table[vector].lock);

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * handler_dispatch
 * ----------------
 * Dispatch interrupt ke handler.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return: 1 jika handled, 0 jika tidak
 */
int handler_dispatch(tak_bertanda32_t vector)
{
    int handled;

    if (!handler_initialized || vector >= JUMLAH_ISR) {
        return 0;
    }

    spinlock_kunci(&vector_table[vector].lock);

    handled = call_handler_chain(vector);

    if (!handled) {
        handler_stats.spurious_count++;
    }

    spinlock_buka(&vector_table[vector].lock);

    return handled;
}

/*
 * handler_get_count
 * -----------------
 * Dapatkan jumlah handler untuk vector.
 *
 * Parameter:
 *   vector - Nomor vector
 *
 * Return: Jumlah handler
 */
tak_bertanda32_t handler_get_count(tak_bertanda32_t vector)
{
    if (!handler_initialized || vector >= JUMLAH_ISR) {
        return 0;
    }

    return vector_table[vector].handler_count;
}

/*
 * handler_get_stats
 * -----------------
 * Dapatkan statistik handler.
 *
 * Parameter:
 *   vector      - Nomor vector
 *   dev_id      - Device ID
 *   call_count  - Pointer untuk call count
 *   error_count - Pointer untuk error count
 *
 * Return: Status operasi
 */
status_t handler_get_stats(tak_bertanda32_t vector, void *dev_id,
                           tak_bertanda32_t *call_count,
                           tak_bertanda32_t *error_count)
{
    handler_entry_t *entry;

    if (!handler_initialized || vector >= JUMLAH_ISR) {
        return STATUS_PARAM_INVALID;
    }

    spinlock_kunci(&vector_table[vector].lock);

    entry = vector_table[vector].handlers;

    while (entry != NULL) {
        if (entry->dev_id == dev_id) {
            if (call_count != NULL) {
                *call_count = entry->call_count;
            }

            if (error_count != NULL) {
                *error_count = entry->error_count;
            }

            spinlock_buka(&vector_table[vector].lock);
            return STATUS_BERHASIL;
        }
        entry = entry->next;
    }

    spinlock_buka(&vector_table[vector].lock);

    return STATUS_TIDAK_DITEMUKAN;
}

/*
 * handler_print_vector
 * --------------------
 * Print informasi handler untuk vector tertentu.
 *
 * Parameter:
 *   vector - Nomor vector
 */
void handler_print_vector(tak_bertanda32_t vector)
{
    handler_entry_t *entry;

    if (!handler_initialized || vector >= JUMLAH_ISR) {
        return;
    }

    kernel_printf("  Vector %lu: %lu handler(s)\n",
                  vector, vector_table[vector].handler_count);

    entry = vector_table[vector].handlers;

    while (entry != NULL) {
        kernel_printf("    - %s (priority=%lu, calls=%lu, errors=%lu)\n",
                      entry->name ? entry->name : "unnamed",
                      entry->priority,
                      entry->call_count,
                      entry->error_count);
        entry = entry->next;
    }
}

/*
 * handler_print_all
 * -----------------
 * Print semua handler yang terdaftar.
 */
void handler_print_all(void)
{
    tak_bertanda32_t i;
    tak_bertanda32_t active_vectors = 0;

    kernel_printf("\n[HANDLER] Interrupt Handler Status:\n");
    kernel_printf("========================================\n");

    for (i = 0; i < JUMLAH_ISR; i++) {
        if (vector_table[i].handler_count > 0) {
            active_vectors++;
            handler_print_vector(i);
        }
    }

    kernel_printf("----------------------------------------\n");
    kernel_printf("  Active vectors: %lu\n", active_vectors);
    kernel_printf("  Total calls:    %lu\n", handler_stats.total_calls);
    kernel_printf("  Total errors:   %lu\n", handler_stats.total_errors);
    kernel_printf("  Spurious:       %lu\n", handler_stats.spurious_count);
    kernel_printf("========================================\n");
}
