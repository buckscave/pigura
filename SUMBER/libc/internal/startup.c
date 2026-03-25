/*
 * PIGURA LIBC - STARTUP.C
 * ========================
 * Kode startup untuk program C runtime initialization.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * File ini berisi fungsi _start yang merupakan entry point
 * pertama yang dipanggil oleh loader sebelum main().
 */

#include <stdlib.h>
#include <stddef.h>

/* ============================================================
 * DEKLARASI EKSTERNAL
 * ============================================================
 */

/* Entry point user program */
extern int main(int argc, char **argv, char **envp);

/* Fungsi inisialisasi internal */
extern void __libc_init(void);
extern void __libc_fini(void);

/* Fungsi untuk constructors/destructors (C++) */
extern void __libc_csu_init(int argc, char **argv, char **envp);
extern void __libc_csu_fini(void);

/* ============================================================
 * PROTOTIPE FUNGSI INTERNAL
 * ============================================================
 */

/* Inisialisasi standard I/O */
extern void __stdio_init(void);

/* Inisialisasi stdlib */
extern void __stdlib_init(void);

/* Cleanup stdio */
extern void __stdio_cleanup(void);

/* Cleanup stdlib */
extern void __stdlib_cleanup(void);

/* Inisialisasi thread-local storage */
extern void __tls_init(void);

/* Dapatkan argumen dari kernel */
extern void __get_args(int *argc, char ***argv, char ***envp);

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* Environment pointer global */
char **__environ = NULL;
char **environ = NULL;

/* Argumen global */
int __argc = 0;
char **__argv = NULL;

/* Program invocation name */
char *__progname = NULL;
char *__progname_full = NULL;

/* Flag untuk menandai inisialisasi */
static int __libc_initialized = 0;

/* Array atexit handlers */
#define ATEXIT_MAX_FUNCS 32
static void (*__atexit_handlers[ATEXIT_MAX_FUNCS])(void);
static int __atexit_count = 0;

/* Array quick_exit handlers */
#define ATEXIT_QUICK_MAX 32
static void (*__quick_exit_handlers[ATEXIT_QUICK_MAX])(void);
static int __quick_exit_count = 0;

/* ============================================================
 * FUNGSI INISIALISASI
 * ============================================================
 */

/*
 * __libc_init - Inisialisasi C runtime library
 *
 * Fungsi ini dipanggil sebelum main() untuk menyiapkan
 * environment runtime.
 */
void __libc_init(void) {
    if (__libc_initialized) {
        return;
    }
    
    /* Inisialisasi variabel global */
    __atexit_count = 0;
    __quick_exit_count = 0;
    
    /* Inisialisasi standard I/O */
    __stdio_init();
    
    /* Inisialisasi stdlib */
    __stdlib_init();
    
    /* Inisialisasi TLS (jika diperlukan) */
    __tls_init();
    
    __libc_initialized = 1;
}

/*
 * __libc_fini - Cleanup C runtime library
 *
 * Fungsi ini dipanggil saat program exit untuk membersihkan
 * resources yang dialokasikan oleh library.
 */
void __libc_fini(void) {
    if (!__libc_initialized) {
        return;
    }
    
    /* Cleanup stdio */
    __stdio_cleanup();
    
    /* Cleanup stdlib */
    __stdlib_cleanup();
    
    __libc_initialized = 0;
}

/* ============================================================
 * ATEXIT HANDLERS
 * ============================================================
 */

/*
 * atexit - Daftarkan fungsi untuk dipanggil saat exit
 *
 * Parameter:
 *   func - Fungsi yang akan didaftarkan
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
int atexit(void (*func)(void)) {
    if (__atexit_count >= ATEXIT_MAX_FUNCS) {
        return -1;
    }
    
    __atexit_handlers[__atexit_count++] = func;
    return 0;
}

/*
 * __run_atexit_handlers - Jalankan semua atexit handlers
 *
 * Handlers dijalankan dalam urutan LIFO (Last In First Out).
 */
void __run_atexit_handlers(void) {
    int i;
    
    /* Jalankan dalam urutan terbalik (LIFO) */
    for (i = __atexit_count - 1; i >= 0; i--) {
        if (__atexit_handlers[i] != NULL) {
            __atexit_handlers[i]();
        }
    }
    
    __atexit_count = 0;
}

/*
 * at_quick_exit - Daftarkan fungsi untuk quick_exit
 *
 * Parameter:
 *   func - Fungsi yang akan didaftarkan
 *
 * Return: 0 jika berhasil, non-zero jika gagal
 */
int at_quick_exit(void (*func)(void)) {
    if (__quick_exit_count >= ATEXIT_QUICK_MAX) {
        return -1;
    }
    
    __quick_exit_handlers[__quick_exit_count++] = func;
    return 0;
}

/*
 * __run_quick_exit_handlers - Jalankan quick_exit handlers
 */
void __run_quick_exit_handlers(void) {
    int i;
    
    for (i = __quick_exit_count - 1; i >= 0; i--) {
        if (__quick_exit_handlers[i] != NULL) {
            __quick_exit_handlers[i]();
        }
    }
    
    __quick_exit_count = 0;
}

/* ============================================================
 * EXIT FUNCTIONS
 * ============================================================
 */

/*
 * exit - Terminasi program dengan cleanup
 *
 * Parameter:
 *   status - Exit status untuk parent process
 */
void exit(int status) {
    /* Jalankan atexit handlers */
    __run_atexit_handlers();
    
    /* Jalankan libc finalizers */
    __libc_fini();
    
    /* Flush dan tutup semua stream */
    __stdio_cleanup();
    
    /* Exit ke kernel */
    _exit(status);
    
    /* Never reached */
    for (;;) {
        /* Infinite loop sebagai fallback */
    }
}

/*
 * quick_exit - Terminasi program dengan quick handlers only
 *
 * Parameter:
 *   status - Exit status
 */
void quick_exit(int status) {
    /* Jalankan quick_exit handlers saja */
    __run_quick_exit_handlers();
    
    /* Exit tanpa cleanup lain */
    _exit(status);
    
    for (;;) {
    }
}

/* ============================================================
 * ENTRY POINT
 * ============================================================
 */

/*
 * _start - Entry point awal dari program
 *
 * Fungsi ini dipanggil oleh loader (atau crt0) dan bertugas:
 * 1. Setup stack dan environment
 * 2. Memanggil __libc_init untuk inisialisasi library
 * 3. Memanggil main() dengan argumen yang tepat
 * 4. Memanggil exit() dengan return value dari main()
 *
 * Catatan: Layout stack saat entry tergantung pada platform:
 * - x86 Linux: argc di [esp], argv di [esp+4], envp di [esp+8]
 * - x86_64: argc di [rsp], argv di [rsp+8], dll
 */
void _start(void) {
    int argc;
    char **argv;
    char **envp;
    int ret;
    
    /* Dapatkan argumen dari stack (platform-specific) */
#if defined(__x86_64__)
    /* x86_64: argc di [rsp], argv di [rsp+8] */
    __asm__ __volatile__(
        "mov (%%rsp), %0\n\t"
        "lea 8(%%rsp), %1\n\t"
        "lea 16(%%rsp, %0, 8), %2\n\t"
        : "=r"(argc), "=r"(argv), "=r"(envp)
    );
#elif defined(__i386__)
    /* x86: argc di [esp], argv di [esp+4] */
    __asm__ __volatile__(
        "mov (%%esp), %0\n\t"
        "lea 4(%%esp), %1\n\t"
        "lea 8(%%esp, %0, 4), %2\n\t"
        : "=r"(argc), "=r"(argv), "=r"(envp)
    );
#elif defined(__aarch64__)
    /* ARM64: x0 = argc, x1 = argv */
    __asm__ __volatile__(
        "mov %0, x0\n\t"
        "mov %1, x1\n\t"
        "add %2, x1, %0, lsl #3\n\t"
        "add %2, %2, #8\n\t"
        : "=r"(argc), "=r"(argv), "=r"(envp)
    );
#else
    /* Default: panggil fungsi helper */
    __get_args(&argc, &argv, &envp);
#endif
    
    /* Set variabel global */
    __argc = argc;
    __argv = argv;
    __environ = envp;
    environ = envp;
    
    /* Set program name */
    if (argc > 0 && argv[0] != NULL) {
        __progname_full = argv[0];
        /* Cari basename dari path */
        char *p = argv[0];
        char *last = p;
        while (*p != '\0') {
            if (*p == '/') {
                last = p + 1;
            }
            p++;
        }
        __progname = last;
    }
    
    /* Inisialisasi C runtime */
    __libc_init();
    
    /* Panggil constructors C++ jika ada */
    __libc_csu_init(argc, argv, envp);
    
    /* Panggil main program */
    ret = main(argc, argv, envp);
    
    /* Exit dengan return code dari main */
    exit(ret);
    
    /* Never reached */
    for (;;) {
    }
}

/* ============================================================
 * STUB FUNGSI
 * ============================================================
 */

/* Stub untuk __libc_csu_init */
void __libc_csu_init(int argc, char **argv, char **envp) {
    (void)argc;
    (void)argv;
    (void)envp;
    /* Kosong - untuk C pure, tidak ada yang perlu dilakukan */
}

/* Stub untuk __libc_csu_fini */
void __libc_csu_fini(void) {
    /* Kosong */
}

/* Stub untuk __tls_init */
void __tls_init(void) {
    /* Kosong - untuk single-threaded, tidak perlu TLS */
}

/* Stub untuk __get_args */
void __get_args(int *argc, char ***argv, char ***envp) {
    /* Default values jika tidak ada argumen */
    static char *empty_argv[] = { "program", NULL };
    static char *empty_envp[] = { NULL };
    
    *argc = 1;
    *argv = empty_argv;
    *envp = empty_envp;
}

/* Stub untuk __stdio_init */
void __stdio_init(void) {
    /* Implementasi di file lain */
}

/* Stub untuk __stdio_cleanup */
void __stdio_cleanup(void) {
    /* Implementasi di file lain */
}

/* Stub untuk __stdlib_init */
void __stdlib_init(void) {
    /* Implementasi di file lain */
}

/* Stub untuk __stdlib_cleanup */
void __stdlib_cleanup(void) {
    /* Implementasi di file lain */
}
