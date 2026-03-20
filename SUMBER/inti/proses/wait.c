/*
 * PIGURA OS - WAIT.C
 * -------------------
 * Implementasi menunggu child proses.
 *
 * Berkas ini berisi fungsi-fungsi untuk menunggu proses child
 * berubah status atau terminasi.
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

/* Flag wait */
#define WAIT_FLAG_NONE          0x00
#define WAIT_FLAG_NOHANG        0x01   /* Non-blocking */
#define WAIT_FLAG_UNTRACED      0x02   /* Report stopped children */
#define WAIT_FLAG_CONTINUED     0x04   /* Report continued children */
#define WAIT_FLAG_EXITED        0x08   /* Report exited children */

/* Status encoding */
#define WAIT_STATUS_EXITED      0x00
#define WAIT_STATUS_SIGNALED    0x01
#define WAIT_STATUS_STOPPED     0x02
#define WAIT_STATUS_CONTINUED   0x03

/* Shift untuk encoding status */
#define WAIT_STATUS_SHIFT       8

/*
 * ============================================================================
 * VARIABEL LOKAL (LOCAL VARIABLES)
 * ============================================================================
 */

/* Statistik wait */
static struct {
    tak_bertanda64_t total_waits;
    tak_bertanda64_t successful;
    tak_bertanda64_t timeouts;
    tak_bertanda64_t interrupted;
} wait_stats = {0};

/* Status inisialisasi */
static bool_t wait_initialized = SALAH;

/*
 * ============================================================================
 * FUNGSI INTERNAL (INTERNAL FUNCTIONS)
 * ============================================================================
 */

/*
 * encode_exit_status
 * ------------------
 * Encode exit status untuk return ke user.
 *
 * Parameter:
 *   proses - Pointer ke proses child
 *
 * Return: Status yang di-encode
 */
static tanda32_t encode_exit_status(proses_t *proses)
{
    tanda32_t status;
    tanda32_t exit_code;
    tanda32_t signal;

    if (proses == NULL) {
        return 0;
    }

    exit_code = proses->exit_code;

    /* Cek apakah exit normal atau karena signal */
    if (exit_code & EXIT_SIGNAL_CORE) {
        /* Exit karena signal */
        signal = (exit_code >> EXIT_SIGNAL_SHIFT) & 0x7F;
        status = (signal << WAIT_STATUS_SHIFT) | WAIT_STATUS_SIGNALED;

        /* Core flag */
        if (exit_code & EXIT_SIGNAL_CORE) {
            status |= 0x80;
        }
    } else {
        /* Exit normal */
        status = ((exit_code & 0xFF) << WAIT_STATUS_SHIFT) | WAIT_STATUS_EXITED;
    }

    return status;
}

/*
 * find_child
 * ----------
 * Cari child proses berdasarkan PID.
 *
 * Parameter:
 *   parent - Pointer ke proses parent
 *   pid    - PID child (-1 untuk any, 0 untuk same group, >0 untuk specific)
 *
 * Return: Pointer ke child, atau NULL
 */
static proses_t *find_child(proses_t *parent, pid_t pid)
{
    proses_t *child;

    if (parent == NULL) {
        return NULL;
    }

    child = parent->children;

    while (child != NULL) {
        if (pid == -1) {
            /* Any child */
            return child;
        } else if (pid == 0) {
            /* Same process group */
            if (child->pgid == parent->pgid) {
                return child;
            }
        } else if (pid > 0) {
            /* Specific child */
            if (child->pid == pid) {
                return child;
            }
        } else {
            /* pid < -1: any child in specified group */
            if (child->pgid == -pid) {
                return child;
            }
        }

        child = child->sibling;
    }

    return NULL;
}

/*
 * find_zombie_child
 * -----------------
 * Cari child zombie.
 *
 * Parameter:
 *   parent - Pointer ke proses parent
 *   pid    - PID child (lihat find_child)
 *   options - Wait options
 *
 * Return: Pointer ke child zombie, atau NULL
 */
static proses_t *find_zombie_child(proses_t *parent, pid_t pid,
                                   tak_bertanda32_t options)
{
    proses_t *child;

    if (parent == NULL) {
        return NULL;
    }

    child = parent->children;

    while (child != NULL) {
        bool_t match = SALAH;

        /* Cek PID match */
        if (pid == -1) {
            match = BENAR;
        } else if (pid == 0) {
            match = (child->pgid == parent->pgid);
        } else if (pid > 0) {
            match = (child->pid == pid);
        } else {
            match = (child->pgid == -pid);
        }

        if (!match) {
            child = child->sibling;
            continue;
        }

        /* Cek status */
        if (child->status == PROSES_STATUS_ZOMBIE) {
            return child;
        }

        /* Cek stopped jika WAIT_FLAG_UNTRACED */
        if ((options & WAIT_FLAG_UNTRACED) &&
            child->status == PROSES_STATUS_STOP) {
            return child;
        }

        /* Cek continued jika WAIT_FLAG_CONTINUED */
        if ((options & WAIT_FLAG_CONTINUED) &&
            (child->flags & PROSES_FLAG_CONTINUED)) {
            return child;
        }

        child = child->sibling;
    }

    return NULL;
}

/*
 * collect_child
 * -------------
 * Kumpulkan status child dan cleanup.
 *
 * Parameter:
 *   child  - Pointer ke child proses
 *   status - Pointer untuk menyimpan exit status
 *
 * Return: PID child
 */
static pid_t collect_child(proses_t *child, tanda32_t *status)
{
    pid_t pid;

    if (child == NULL) {
        return -1;
    }

    pid = child->pid;

    /* Encode dan return status */
    if (status != NULL) {
        *status = encode_exit_status(child);
    }

    /* Mark sebagai collected */
    child->wait_collected = BENAR;

    return pid;
}

/*
 * has_children
 * ------------
 * Cek apakah proses memiliki children.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: BENAR jika memiliki children
 */
static bool_t has_children(proses_t *proses)
{
    if (proses == NULL) {
        return SALAH;
    }

    return (proses->children != NULL);
}

/*
 * has_zombie_children
 * -------------------
 * Cek apakah proses memiliki zombie children.
 *
 * Parameter:
 *   proses - Pointer ke proses
 *
 * Return: BENAR jika memiliki zombie children
 */
static bool_t has_zombie_children(proses_t *proses)
{
    proses_t *child;

    if (proses == NULL) {
        return SALAH;
    }

    child = proses->children;
    while (child != NULL) {
        if (child->status == PROSES_STATUS_ZOMBIE) {
            return BENAR;
        }
        child = child->sibling;
    }

    return SALAH;
}

/*
 * ============================================================================
 * FUNGSI PUBLIK (PUBLIC FUNCTIONS)
 * ============================================================================
 */

/*
 * wait_init
 * ---------
 * Inisialisasi subsistem wait.
 *
 * Return: Status operasi
 */
status_t wait_init(void)
{
    if (wait_initialized) {
        return STATUS_SUDAH_ADA;
    }

    kernel_memset(&wait_stats, 0, sizeof(wait_stats));
    wait_initialized = BENAR;

    kernel_printf("[WAIT] Wait subsystem initialized\n");

    return STATUS_BERHASIL;
}

/*
 * do_wait
 * -------
 * Tunggu child proses.
 *
 * Parameter:
 *   pid     - PID child (-1 untuk any)
 *   status  - Pointer untuk exit status
 *   options - Wait options
 *   rusage  - Pointer untuk resource usage (belum diimplementasi)
 *
 * Return: PID child yang berubah, atau -1 jika error
 */
pid_t do_wait(pid_t pid, tanda32_t *status, tak_bertanda32_t options,
              void *rusage)
{
    proses_t *parent;
    proses_t *child;
    pid_t result;

    if (!wait_initialized) {
        return -1;
    }

    parent = proses_get_current();
    if (parent == NULL) {
        return -1;
    }

    wait_stats.total_waits++;

    /* Cek apakah memiliki children */
    if (!has_children(parent)) {
        return -ERROR_NOCHILD;
    }

    /* Cari zombie child */
    child = find_zombie_child(parent, pid, options);

    if (child != NULL) {
        /* Found zombie, collect */
        result = collect_child(child, status);
        wait_stats.successful++;
        return result;
    }

    /* Tidak ada zombie, cek NOHANG */
    if (options & WAIT_FLAG_NOHANG) {
        return 0;  /* No child ready */
    }

    /* Tidak ada zombie, block sampai ada */
    while (BENAR) {
        /* Set wait state */
        parent->wait_state = WAIT_STATE_WAITING;
        parent->wait_pid = pid;
        parent->wait_options = options;

        /* Block */
        scheduler_block(BLOCK_REASON_WAIT);

        /* Cek lagi untuk zombie */
        child = find_zombie_child(parent, pid, options);

        if (child != NULL) {
            result = collect_child(child, status);
            parent->wait_state = WAIT_STATE_NONE;
            wait_stats.successful++;
            return result;
        }

        /* Cek jika di-interrupt */
        if (signal_pending(parent)) {
            parent->wait_state = WAIT_STATE_NONE;
            wait_stats.interrupted++;
            return -ERROR_INTR;
        }

        /* Cek jika tidak punya children lagi */
        if (!has_children(parent)) {
            parent->wait_state = WAIT_STATE_NONE;
            return -ERROR_NOCHILD;
        }
    }

    return -1;
}

/*
 * sys_waitpid
 * -----------
 * System call waitpid.
 *
 * Parameter:
 *   pid     - PID child
 *   status  - Pointer untuk exit status
 *   options - Wait options
 *
 * Return: PID child atau -1
 */
pid_t sys_waitpid(pid_t pid, tanda32_t *status, tak_bertanda32_t options)
{
    tanda32_t local_status = 0;
    pid_t result;

    /* Validasi pointer user */
    if (status != NULL && !validasi_pointer_user(status)) {
        return -ERROR_FAULT;
    }

    result = do_wait(pid, &local_status, options, NULL);

    /* Copy status ke userspace */
    if (status != NULL && result > 0) {
        *status = local_status;
    }

    return result;
}

/*
 * sys_wait
 * --------
 * System call wait.
 *
 * Parameter:
 *   status - Pointer untuk exit status
 *
 * Return: PID child atau -1
 */
pid_t sys_wait(tanda32_t *status)
{
    return sys_waitpid(-1, status, 0);
}

/*
 * sys_waitid
 * ----------
 * System call waitid.
 *
 * Parameter:
 *   idtype  - Tipe ID (P_ALL, P_PID, P_PGID)
 *   id      - ID
 *   infop   - Pointer ke siginfo_t
 *   options - Wait options
 *
 * Return: 0 jika berhasil, -1 jika error
 */
tanda32_t sys_waitid(tak_bertanda32_t idtype, pid_t id,
                     siginfo_t *infop, tak_bertanda32_t options)
{
    proses_t *parent;
    proses_t *child;
    pid_t target_pid;
    tanda32_t status;
    pid_t result;

    if (infop != NULL && !validasi_pointer_user(infop)) {
        return -ERROR_FAULT;
    }

    parent = proses_get_current();
    if (parent == NULL) {
        return -ERROR_NOCHILD;
    }

    /* Tentukan target PID */
    switch (idtype) {
        case P_ALL:
            target_pid = -1;
            break;
        case P_PID:
            target_pid = id;
            break;
        case P_PGID:
            target_pid = -id;
            break;
        default:
            return -ERROR_INVALID;
    }

    result = do_wait(target_pid, &status, options, NULL);

    if (result < 0) {
        return result;
    }

    /* Isi siginfo */
    if (infop != NULL) {
        child = proses_cari(result);
        if (child != NULL) {
            infop->si_signo = SIGCHLD;
            infop->si_pid = child->pid;
            infop->si_uid = child->uid;
            infop->si_status = status >> WAIT_STATUS_SHIFT;
            infop->si_code = CLD_EXITED;
        }
    }

    return 0;
}

/*
 * sys_wait4
 * ---------
 * System call wait4 (BSD compatible).
 *
 * Parameter:
 *   pid     - PID child
 *   status  - Pointer untuk exit status
 *   options - Wait options
 *   rusage  - Pointer untuk resource usage
 *
 * Return: PID child atau -1
 */
pid_t sys_wait4(pid_t pid, tanda32_t *status, tak_bertanda32_t options,
                void *rusage)
{
    tanda32_t local_status = 0;
    pid_t result;

    /* Validasi pointer */
    if (status != NULL && !validasi_pointer_user(status)) {
        return -ERROR_FAULT;
    }

    if (rusage != NULL && !validasi_pointer_user(rusage)) {
        return -ERROR_FAULT;
    }

    result = do_wait(pid, &local_status, options, rusage);

    /* Copy status */
    if (status != NULL && result > 0) {
        *status = local_status;
    }

    /* TODO: Fill rusage jika diminta */

    return result;
}

/*
 * wait_get_stats
 * --------------
 * Dapatkan statistik wait.
 *
 * Parameter:
 *   total       - Pointer untuk total wait
 *   successful  - Pointer untuk wait berhasil
 *   timeouts    - Pointer untuk timeout
 *   interrupted - Pointer untuk interrupted
 */
void wait_get_stats(tak_bertanda64_t *total, tak_bertanda64_t *successful,
                    tak_bertanda64_t *timeouts, tak_bertanda64_t *interrupted)
{
    if (total != NULL) {
        *total = wait_stats.total_waits;
    }

    if (successful != NULL) {
        *successful = wait_stats.successful;
    }

    if (timeouts != NULL) {
        *timeouts = wait_stats.timeouts;
    }

    if (interrupted != NULL) {
        *interrupted = wait_stats.interrupted;
    }
}

/*
 * wait_print_stats
 * ----------------
 * Print statistik wait.
 */
void wait_print_stats(void)
{
    kernel_printf("\n[WAIT] Statistik Wait:\n");
    kernel_printf("========================================\n");
    kernel_printf("  Total Waits    : %lu\n", wait_stats.total_waits);
    kernel_printf("  Successful     : %lu\n", wait_stats.successful);
    kernel_printf("  Timeouts       : %lu\n", wait_stats.timeouts);
    kernel_printf("  Interrupted    : %lu\n", wait_stats.interrupted);
    kernel_printf("========================================\n");
}

/*
 * wait_wakeup_parent
 * ------------------
 * Wake up parent yang menunggu child ini.
 *
 * Parameter:
 *   child - Pointer ke child proses
 */
void wait_wakeup_parent(proses_t *child)
{
    proses_t *parent;

    if (child == NULL) {
        return;
    }

    parent = proses_cari(child->ppid);
    if (parent == NULL) {
        return;
    }

    /* Cek jika parent menunggu */
    if (parent->wait_state != WAIT_STATE_NONE) {
        /* Cek jika menunggu child ini */
        if (parent->wait_pid == -1 ||
            parent->wait_pid == child->pid ||
            (parent->wait_pid == 0 && parent->pgid == child->pgid) ||
            (parent->wait_pid < -1 && -parent->wait_pid == child->pgid)) {

            /* Wake up parent */
            scheduler_unblock(parent);
        }
    }
}
