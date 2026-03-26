/*
 * PIGURA LIBC - STDLIB/EXIT.C
 * ============================
 * Implementasi fungsi terminasi program.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 2.0 - Disinkronkan dengan syscall Pigura OS
 *
 * Fungsi yang diimplementasikan:
 *   - abort()  : Terminasi abnormal program
 *   - _exit()  : Terminasi tanpa cleanup
 *   - _Exit()  : Terminasi C99 tanpa cleanup
 *   - quick_exit() : Terminasi dengan quick handlers
 *   - atexit() : Daftarkan exit handler
 *   - at_quick_exit() : Daftarkan quick exit handler
 */

#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/syscall.h>   /* Syscall numbers dari header terpusat */

/* ============================================================
 * KONFIGURASI
 * ============================================================
 */

/* Jumlah maksimum handler atexit */
#define ATEXIT_MAX_FUNCS    32

/* Jumlah maksimum handler quick_exit */
#define AT_QUICK_EXIT_MAX   32

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Tabel handler atexit */
static void (*atexit_funcs[ATEXIT_MAX_FUNCS])(void);

/* Jumlah handler atexit terdaftar */
static int atexit_count = 0;

/* Tabel handler quick_exit */
static void (*at_quick_exit_funcs[AT_QUICK_EXIT_MAX])(void);

/* Jumlah handler quick_exit terdaftar */
static int at_quick_exit_count = 0;

/* Flag untuk mencegah recursive exit */
static int in_exit = 0;

/* ============================================================
 * DEKLARASI FUNGSI EKSTERNAL
 * ============================================================
 */

/* Fungsi cleanup dari stdio */
extern void __stdio_cleanup(void);

/* Fungsi cleanup dari stdlib */
extern void __stdlib_cleanup(void);

/* ============================================================
 * HELPER FUNCTION
 * ============================================================
 */

/*
 * __syscall_exit - Wrapper untuk syscall exit
 *
 * Parameter:
 *   status - Exit status
 *
 * Tidak kembali.
 */
static void __syscall_exit(int status) __attribute__((noreturn));
static void __syscall_exit(int status) {
    /* Gunakan syscall exit Pigura OS */
    syscall1(SYS_EXIT, (long)status);

    /* Never reached, but added for noreturn */
    for (;;) {
        /* Infinite loop as fallback */
    }
}

/* ============================================================
 * IMPLEMENTASI FUNGSI
 * ============================================================
 */

/*
 * atexit - Daftarkan fungsi untuk dipanggil saat exit
 */
int atexit(void (*func)(void)) {
    /* Validasi parameter */
    if (func == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Cek kapasitas */
    if (atexit_count >= ATEXIT_MAX_FUNCS) {
        errno = ENOMEM;
        return -1;
    }

    /* Tambahkan ke tabel */
    atexit_funcs[atexit_count++] = func;

    return 0;
}

/*
 * at_quick_exit - Daftarkan fungsi untuk quick_exit
 */
int at_quick_exit(void (*func)(void)) {
    /* Validasi parameter */
    if (func == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* Cek kapasitas */
    if (at_quick_exit_count >= AT_QUICK_EXIT_MAX) {
        errno = ENOMEM;
        return -1;
    }

    /* Tambahkan ke tabel */
    at_quick_exit_funcs[at_quick_exit_count++] = func;

    return 0;
}

/*
 * __run_atexit_handlers - Jalankan semua handler atexit
 *
 * Handler dijalankan dalam urutan LIFO (terakhir didaftar,
 * pertama dijalankan).
 */
void __run_atexit_handlers(void) {
    int i;

    /* Jalankan dalam urutan terbalik (LIFO) */
    for (i = atexit_count - 1; i >= 0; i--) {
        if (atexit_funcs[i] != NULL) {
            atexit_funcs[i]();
        }
    }

    atexit_count = 0;
}

/*
 * __run_quick_exit_handlers - Jalankan semua handler quick_exit
 *
 * Handler dijalankan dalam urutan LIFO.
 */
void __run_quick_exit_handlers(void) {
    int i;

    /* Jalankan dalam urutan terbalik (LIFO) */
    for (i = at_quick_exit_count - 1; i >= 0; i--) {
        if (at_quick_exit_funcs[i] != NULL) {
            at_quick_exit_funcs[i]();
        }
    }

    at_quick_exit_count = 0;
}

/*
 * exit - Hentikan program secara normal
 *
 * Parameter:
 *   status - Exit status
 *
 * Memanggil fungsi terdaftar dengan atexit() dan flush
 * semua stream sebelum menghentikan program.
 */
void exit(int status) {
    /* Cegah recursive exit */
    if (in_exit) {
        __syscall_exit(status);
    }
    in_exit = 1;

    /* Jalankan handler atexit */
    __run_atexit_handlers();

    /* Cleanup stdio (flush buffers) */
    __stdio_cleanup();

    /* Cleanup stdlib */
    __stdlib_cleanup();

    /* Panggil syscall exit */
    __syscall_exit(status);
}

/*
 * _exit - Hentikan program tanpa cleanup
 *
 * Parameter:
 *   status - Exit status
 *
 * Menghentikan program tanpa memanggil atexit handlers
 * dan tanpa flush stdio buffers.
 */
void _exit(int status) {
    /* Langsung panggil syscall */
    __syscall_exit(status);
}

/*
 * _Exit - Hentikan program tanpa cleanup (C99)
 *
 * Parameter:
 *   status - Exit status
 *
 * Sama seperti _exit(), tidak memanggil handler.
 */
void _Exit(int status) {
    /* Langsung panggil syscall */
    __syscall_exit(status);
}

/*
 * quick_exit - Hentikan program dengan quick handlers (C11)
 *
 * Parameter:
 *   status - Exit status
 *
 * Memanggil handler yang terdaftar dengan at_quick_exit(),
 * lalu memanggil _Exit().
 */
void quick_exit(int status) {
    /* Jalankan handler quick_exit */
    __run_quick_exit_handlers();

    /* Terminasi tanpa cleanup lainnya */
    _Exit(status);
}

/*
 * abort - Hentikan program secara abnormal
 *
 * Menghasilkan sinyal SIGABRT dan menghentikan program.
 * Jika sinyal ditangkap dan handler kembali, program tetap
 * dihentikan.
 */
void abort(void) {
    /* Cegah recursive abort */
    static int in_abort = 0;

    if (in_abort) {
        /* Sudah di dalam abort, terminasi segera */
        __syscall_exit(134);  /* 128 + SIGABRT */
    }
    in_abort = 1;

    /* Reset handler SIGABRT ke default */
    signal(SIGABRT, SIG_DFL);

    /* Kirim sinyal SIGABRT ke diri sendiri */
    raise(SIGABRT);

    /* Jika kita sampai di sini, handler SIGABRT kembali.
     * Reset handler dan kirim lagi untuk memastikan terminasi. */
    signal(SIGABRT, SIG_DFL);
    raise(SIGABRT);

    /* Terakhir, terminasi langsung */
    __syscall_exit(134);
}
