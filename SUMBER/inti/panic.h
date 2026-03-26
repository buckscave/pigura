/*
 * PIGURA OS - PANIC.H
 * ====================
 * Definisi dan deklarasi fungsi kernel panic.
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani kondisi error fatal
 * yang tidak dapat dipulihkan. Kernel panic akan menghentikan sistem
 * dan menampilkan informasi error untuk debugging.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua fungsi dideklarasikan secara eksplisit
 *
 * Versi: 2.2
 * Tanggal: 2025
 */

#ifndef INTI_PANIC_H
#define INTI_PANIC_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "types.h"
#include "stdarg.h"

/*
 * ===========================================================================
 * KONSTANTA PANIC (PANIC CONSTANTS)
 * ===========================================================================
 * Kode error untuk berbagai kondisi panic.
 */

/* Kode error panic - umum */
#define PANIC_KODE_TIDAK_DIKETAHUI 0x0000
#define PANIC_KODE_ASSERT 0x0001
#define PANIC_KODE_NULL_POINTER 0x0002
#define PANIC_KODE_OUT_OF_MEMORY 0x0003
#define PANIC_KODE_STACK_OVERFLOW 0x0004
#define PANIC_KODE_STACK_UNDERFLOW 0x0005
#define PANIC_KODE_STACK_CORRUPT 0x0006

/* Kode error panic - CPU exception */
#define PANIC_KODE_PAGE_FAULT 0x0010
#define PANIC_KODE_GENERAL_PROTECTION 0x0011
#define PANIC_KODE_DOUBLE_FAULT 0x0012
#define PANIC_KODE_INVALID_TSS 0x0013
#define PANIC_KODE_SEGMENT_NOT_PRESENT 0x0014
#define PANIC_KODE_INVALID_OPCODE 0x0015
#define PANIC_KODE_DEVICE_NOT_AVAIL 0x0016
#define PANIC_KODE_ALIGNMENT_CHECK 0x0017
#define PANIC_KODE_MACHINE_CHECK 0x0018
#define PANIC_KODE_DIVIDE_ERROR 0x0019
#define PANIC_KODE_OVERFLOW 0x001A
#define PANIC_KODE_BOUND_RANGE 0x001B
#define PANIC_KODE_FPU_EXCEPTION 0x001C
#define PANIC_KODE_SIMD_EXCEPTION 0x001D
#define PANIC_KODE_BREAKPOINT 0x001E
#define PANIC_KODE_NMI 0x001F

/* Kode error panic - kernel */
#define PANIC_KODE_INTERRUPT 0x0020
#define PANIC_KODE_SYSCALL_INVALID 0x0030
#define PANIC_KODE_HAL_ERROR 0x0040
#define PANIC_KODE_VFS_ERROR 0x0050
#define PANIC_KODE_DRIVER_ERROR 0x0060
#define PANIC_KODE_SCHEDULER_ERROR 0x0070
#define PANIC_KODE_MEMORY_ERROR 0x0080

/* Kode error panic - user-defined */
#define PANIC_KODE_USER_DEFINED 0x1000

/* Panjang maksimum pesan panic */
#define PANIC_PESAN_MAX 256

/* Panjang maksimum nama file */
#define PANIC_FILE_MAX 128

/* Panjang maksimum nama fungsi */
#define PANIC_FUNC_MAX 64

/* Panjang maksimum stack trace */
#define PANIC_STACK_TRACE_MAX 16

/*
 * ===========================================================================
 * STRUKTUR DATA REGISTER CONTEXT - MULTI-ARSITEKTUR
 * ===========================================================================
 * Struktur untuk menyimpan konteks register saat exception.
 * Implementasi berbeda untuk setiap arsitektur.
 * 
 * CATATAN PENTING: Kondisional harus EXCLUSIVE (mutually exclusive)
 * Gunakan #if ... #elif ... #elif ... #else ... #endif
 */

/* -------------------------------------------------------------------------
 * ARSITEKTUR x86 (32-bit)
 * ------------------------------------------------------------------------- */
#if defined(ARSITEKTUR_X86)

typedef struct register_context {
    /* Segment registers */
    tak_bertanda32_t gs;
    tak_bertanda32_t fs;
    tak_bertanda32_t es;
    tak_bertanda32_t ds;

    /* General purpose registers (pushed by handler) */
    tak_bertanda32_t edi;
    tak_bertanda32_t esi;
    tak_bertanda32_t ebp;
    tak_bertanda32_t esp;

    tak_bertanda32_t ebx;
    tak_bertanda32_t edx;
    tak_bertanda32_t ecx;
    tak_bertanda32_t eax;

    /* Interrupt info (pushed by CPU) */
    tak_bertanda32_t int_no;
    tak_bertanda32_t err_code;

    /* Control flow (pushed by CPU) */
    tak_bertanda32_t eip;
    tak_bertanda32_t cs;
    tak_bertanda32_t eflags;

    /* User stack (only for privilege change) */
    tak_bertanda32_t useresp;
    tak_bertanda32_t ss;
} register_context_t;

/* -------------------------------------------------------------------------
 * ARSITEKTUR x86_64 (64-bit)
 * ------------------------------------------------------------------------- */
#elif defined(ARSITEKTUR_X86_64)

typedef struct register_context {
    /* General purpose registers */
    tak_bertanda64_t rax;
    tak_bertanda64_t rbx;
    tak_bertanda64_t rcx;
    tak_bertanda64_t rdx;
    tak_bertanda64_t rsi;
    tak_bertanda64_t rdi;
    tak_bertanda64_t rbp;
    tak_bertanda64_t rsp;
    tak_bertanda64_t r8;
    tak_bertanda64_t r9;
    tak_bertanda64_t r10;
    tak_bertanda64_t r11;
    tak_bertanda64_t r12;
    tak_bertanda64_t r13;
    tak_bertanda64_t r14;
    tak_bertanda64_t r15;

    /* Interrupt info */
    tak_bertanda64_t int_no;
    tak_bertanda64_t err_code;

    /* Control flow */
    tak_bertanda64_t rip;
    tak_bertanda64_t cs;
    tak_bertanda64_t rflags;
    tak_bertanda64_t rsp_user;
    tak_bertanda64_t ss;
} register_context_t;

/* -------------------------------------------------------------------------
 * ARSITEKTUR ARM64 (AArch64)
 * ------------------------------------------------------------------------- */
#elif defined(ARSITEKTUR_ARM64)

typedef struct register_context {
    /* General purpose registers x0-x30 */
    tak_bertanda64_t x[31];
    tak_bertanda64_t sp;         /* Stack pointer */
    tak_bertanda64_t pc;         /* Program counter */
    tak_bertanda64_t pstate;     /* Processor state */
    
    /* Exception info */
    tak_bertanda32_t exception_type;
    tak_bertanda32_t fault_address;
} register_context_t;

/* -------------------------------------------------------------------------
 * ARSITEKTUR ARM / ARMv7 (32-bit)
 * ------------------------------------------------------------------------- */
#elif defined(ARSITEKTUR_ARM) || defined(ARSITEKTUR_ARMV7)

typedef struct register_context {
    /* General purpose registers r0-r15 (PC is r15) */
    tak_bertanda32_t r[16];
    tak_bertanda32_t cpsr;       /* Current program status register */
    tak_bertanda32_t spsr;       /* Saved program status register */
    
    /* Exception info */
    tak_bertanda32_t exception_type;
    tak_bertanda32_t fault_address;
} register_context_t;

/* -------------------------------------------------------------------------
 * ARSITEKTUR TIDAK DIKENAL - FALLBACK
 * ------------------------------------------------------------------------- */
#else

typedef struct register_context {
    tak_bertanda64_t pc;         /* Program counter */
    tak_bertanda64_t sp;         /* Stack pointer */
    tak_bertanda32_t exception_no;
    tak_bertanda32_t error_code;
    tak_bertanda64_t reserved[16];
} register_context_t;

#endif /* ARSITEKTUR */

/*
 * Alias untuk kompatibilitas
 * interrupt_frame_t adalah alias untuk register_context_t
 * Digunakan oleh syscall handler dan interrupt handler
 */
typedef register_context_t interrupt_frame_t;

/*
 * ===========================================================================
 * STRUKTUR DATA PANIC (PANIC DATA STRUCTURES)
 * ===========================================================================
 * Struktur data untuk menyimpan informasi panic.
 */

/* Struktur informasi stack frame */
typedef struct {
    uintptr_t pc;
    uintptr_t fp;
} stack_frame_t;

/* Struktur informasi panic */
typedef struct {
    tak_bertanda16_t kode;              /* Kode error */
    tak_bertanda16_t bendera;           /* Flag tambahan */
    tak_bertanda32_t baris;             /* Nomor baris */
    char file[PANIC_FILE_MAX];          /* Nama file */
    char fungsi[PANIC_FUNC_MAX];        /* Nama fungsi */
    char pesan[PANIC_PESAN_MAX];        /* Pesan error */
    uintptr_t alamat_fault;             /* Alamat fault */
    tak_bertanda32_t error_code;        /* Error code CPU */
    tak_bertanda32_t nomor_vector;      /* Nomor vector interrupt */
    tak_bertanda64_t timestamp;         /* Waktu panic */
    pid_t pid;                          /* PID proses */
    tid_t tid;                          /* TID thread */
} panic_info_t;

/* Flag untuk panic_info_t */
#define PANIC_FLAG_DUMP_REGISTERS 0x0001
#define PANIC_FLAG_DUMP_STACK 0x0002
#define PANIC_FLAG_DUMP_MEMORY 0x0004
#define PANIC_FLAG_NESTED 0x0010
#define PANIC_FLAG_HALTED 0x0020

/*
 * ===========================================================================
 * DEKLARASI FUNGSI PANIC UTAMA (MAIN PANIC FUNCTIONS)
 * ===========================================================================
 */

/*
 * kernel_panic
 * ------------
 * Fungsi utama untuk menangani kernel panic.
 * Fungsi ini tidak akan pernah return.
 *
 * Parameter:
 *   file   - Nama file tempat panic terjadi
 *   baris  - Nomor baris tempat panic terjadi
 *   format - Format string pesan error
 *   ...    - Argumen format
 */
void kernel_panic(const char *file, int baris, const char *format, ...)
    TIDAK_RETURN;

/*
 * kernel_panic_kode
 * -----------------
 * Kernel panic dengan kode error spesifik.
 *
 * Parameter:
 *   kode   - Kode error panic
 *   file   - Nama file
 *   baris  - Nomor baris
 *   format - Format string
 *   ...    - Argumen format
 */
void kernel_panic_kode(tak_bertanda16_t kode, const char *file, int baris,
                       const char *format, ...) TIDAK_RETURN;

/*
 * kernel_panic_fungsi
 * -------------------
 * Kernel panic dengan informasi fungsi.
 *
 * Parameter:
 *   kode   - Kode error
 *   file   - Nama file
 *   baris  - Nomor baris
 *   fungsi - Nama fungsi
 *   format - Format string
 *   ...    - Argumen format
 */
void kernel_panic_fungsi(tak_bertanda16_t kode, const char *file, int baris,
                         const char *fungsi, const char *format, ...)
    TIDAK_RETURN;

/*
 * kernel_panic_register
 * ---------------------
 * Kernel panic dengan informasi register lengkap.
 *
 * Parameter:
 *   info - Pointer ke struktur panic_info_t
 */
void kernel_panic_register(const panic_info_t *info) TIDAK_RETURN;

/*
 * kernel_panic_exception
 * ----------------------
 * Kernel panic dari exception handler.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register saat exception
 */
void kernel_panic_exception(const register_context_t *ctx) TIDAK_RETURN;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI HALT DAN REBOOT (HALT AND REBOOT FUNCTIONS)
 * ===========================================================================
 */

/*
 * kernel_halt
 * -----------
 * Hentikan sistem tanpa pesan error.
 * Fungsi ini tidak akan pernah return.
 */
void kernel_halt(void) TIDAK_RETURN;

/*
 * kernel_reboot
 * -------------
 * Reboot sistem.
 *
 * Parameter:
 *   force - Jika BENAR, lakukan hard reset
 */
void kernel_reboot(int force) TIDAK_RETURN;

/*
 * kernel_shutdown
 * ---------------
 * Shutdown sistem dengan aman.
 *
 * Parameter:
 *   reboot - 0 = power off, 1 = reboot
 */
void kernel_shutdown(int reboot) TIDAK_RETURN;

/*
 * ===========================================================================
 * DEKLARASI FUNGSI DUMP (DUMP FUNCTIONS)
 * ===========================================================================
 */

/*
 * panic_dump_register
 * -------------------
 * Tampilkan dump register ke console.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register
 */
void panic_dump_register(const register_context_t *ctx);

/*
 * panic_dump_stack
 * ----------------
 * Tampilkan dump stack memory.
 *
 * Parameter:
 *   esp   - Pointer stack
 *   depth - Jumlah frame yang ditampilkan
 */
void panic_dump_stack(uintptr_t esp, int depth);

/*
 * panic_dump_stack_trace
 * ----------------------
 * Tampilkan stack trace (call stack).
 *
 * Parameter:
 *   fp    - Frame pointer awal
 *   depth - Jumlah frame maksimum
 */
void panic_dump_stack_trace(uintptr_t fp, int depth);

/*
 * panic_dump_memory
 * -----------------
 * Tampilkan dump memory dalam format hex.
 *
 * Parameter:
 *   addr - Alamat awal
 *   len  - Panjang dalam byte
 */
void panic_dump_memory(void *addr, ukuran_t len);

/*
 * panic_dump_memory_ascii
 * -----------------------
 * Tampilkan dump memory dengan ASCII.
 *
 * Parameter:
 *   addr - Alamat awal
 *   len  - Panjang dalam byte
 */
void panic_dump_memory_ascii(void *addr, ukuran_t len);

/*
 * ===========================================================================
 * DEKLARASI FUNGSI HELPER (HELPER FUNCTIONS)
 * ===========================================================================
 */

/*
 * panic_set_info
 * --------------
 * Set informasi panic tambahan.
 *
 * Parameter:
 *   info - Pointer ke struktur panic_info_t
 */
void panic_set_info(panic_info_t *info);

/*
 * panic_get_info
 * --------------
 * Dapatkan informasi panic terakhir.
 *
 * Return: Pointer ke struktur panic_info_t
 */
const panic_info_t *panic_get_info(void);

/*
 * panic_set_callback
 * ------------------
 * Set callback yang dipanggil saat panic.
 *
 * Parameter:
 *   callback - Fungsi callback
 */
void panic_set_callback(void (*callback)(const panic_info_t *));

/*
 * panic_is_active
 * ---------------
 * Cek apakah sedang dalam kondisi panic.
 *
 * Return: BENAR jika panic aktif
 */
bool_t panic_is_active(void);

/*
 * ===========================================================================
 * MAKRO PANIC (PANIC MACROS)
 * ===========================================================================
 * Makro untuk memanggil panic dengan informasi otomatis.
 */

/*
 * PANIC
 * -----
 * Macro untuk memanggil kernel_panic dengan informasi file dan baris.
 */
#define PANIC(fmt, ...) \
    kernel_panic(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

/*
 * PANIC_KODE
 * ----------
 * Macro untuk panic dengan kode error spesifik.
 */
#define PANIC_KODE(kode, fmt, ...) \
    kernel_panic_kode(kode, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/*
 * PANIC_FUNC
 * ----------
 * Macro untuk panic dengan informasi fungsi.
 */
#define PANIC_FUNC(fmt, ...) \
    kernel_panic_fungsi(PANIC_KODE_TIDAK_DIKETAHUI, __FILE__, __LINE__, \
                        __func__, fmt, ##__VA_ARGS__)

/*
 * PANIC_IF
 * --------
 * Panic jika kondisi benar.
 */
#define PANIC_IF(kondisi, fmt, ...) \
    do { \
        if (kondisi) { \
            kernel_panic(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

/*
 * PANIC_UNLESS
 * ------------
 * Panic jika kondisi salah.
 */
#define PANIC_UNLESS(kondisi, fmt, ...) \
    do { \
        if (!(kondisi)) { \
            kernel_panic(__FILE__, __LINE__, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

/*
 * PANIC_NULL
 * ----------
 * Panic jika pointer NULL.
 */
#define PANIC_NULL(ptr, fmt, ...) \
    do { \
        if ((ptr) == NULL) { \
            kernel_panic_kode(PANIC_KODE_NULL_POINTER, __FILE__, __LINE__, \
                              "Pointer NULL: " fmt, ##__VA_ARGS__); \
        } \
    } while (0)

/*
 * PANIC_OOM
 * ---------
 * Panic untuk kondisi out of memory.
 */
#define PANIC_OOM() \
    kernel_panic_kode(PANIC_KODE_OUT_OF_MEMORY, __FILE__, __LINE__, \
                      "Kehabisan memori")

/*
 * PANIC_NOT_IMPLEMENTED
 * ---------------------
 * Panic untuk fungsi yang belum diimplementasi.
 */
#define PANIC_NOT_IMPLEMENTED() \
    kernel_panic_fungsi(PANIC_KODE_TIDAK_DIKETAHUI, __FILE__, __LINE__, \
                        __func__, "Fungsi belum diimplementasi")

/*
 * PANIC_INVALID_STATE
 * -------------------
 * Panic untuk state tidak valid.
 */
#define PANIC_INVALID_STATE(fmt, ...) \
    kernel_panic_fungsi(PANIC_KODE_TIDAK_DIKETAHUI, __FILE__, __LINE__, \
                        __func__, "State tidak valid: " fmt, ##__VA_ARGS__)

/*
 * PANIC_BOUNDS
 * ------------
 * Panic untuk index di luar batas.
 */
#define PANIC_BOUNDS(idx, min_val, maks_val) \
    kernel_panic(__FILE__, __LINE__, \
                 "Index di luar batas: %ld (valid: %ld-%ld)", \
                 (long)(idx), (long)(min_val), (long)(maks_val))

/*
 * PANIC_OVERFLOW
 * --------------
 * Panic untuk overflow.
 */
#define PANIC_OVERFLOW(fmt, ...) \
    kernel_panic_kode(PANIC_KODE_STACK_OVERFLOW, __FILE__, __LINE__, \
                      "Overflow: " fmt, ##__VA_ARGS__)

/*
 * ===========================================================================
 * MAKRO ASSERTION (ASSERTION MACROS)
 * ===========================================================================
 * Makro untuk assertion pada runtime.
 */

/*
 * BUILD_ASSERT
 * ------------
 * Assertion pada compile-time (C90 compliant).
 */
#define BUILD_ASSERT(cond) \
    typedef char __build_assert_##__LINE__[(cond) ? 1 : -1] DIGUNAKAN

/*
 * BUILD_ASSERT_MSG
 * ----------------
 * Assertion pada compile-time dengan pesan.
 */
#define BUILD_ASSERT_MSG(cond, msg) \
    typedef char __build_assert_##msg[(cond) ? 1 : -1] DIGUNAKAN

/*
 * VERIFY
 * ------
 * Verifikasi kondisi (selalu aktif, tidak seperti assert).
 */
#define VERIFY(cond) \
    do { \
        if (!(cond)) { \
            kernel_panic_kode(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                              "Verifikasi gagal: " #cond); \
        } \
    } while (0)

/*
 * VERIFY_MSG
 * ----------
 * Verifikasi dengan pesan kustom.
 */
#define VERIFY_MSG(cond, msg) \
    do { \
        if (!(cond)) { \
            kernel_panic_kode(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                              "Verifikasi gagal: %s", msg); \
        } \
    } while (0)

/*
 * VERIFY_PTR
 * ----------
 * Verifikasi pointer tidak NULL.
 */
#define VERIFY_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            kernel_panic_kode(PANIC_KODE_NULL_POINTER, __FILE__, __LINE__, \
                              "Pointer NULL: " #ptr); \
        } \
    } while (0)

/*
 * VERIFY_RANGE
 * ------------
 * Verifikasi nilai dalam rentang.
 */
#define VERIFY_RANGE(val, min_val, maks_val) \
    do { \
        if ((val) < (min_val) || (val) > (maks_val)) { \
            kernel_panic_kode(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                              "Nilai di luar rentang: %ld (valid: %ld-%ld)", \
                              (long)(val), (long)(min_val), (long)(maks_val)); \
        } \
    } while (0)

/*
 * ===========================================================================
 * MAKRO ASSERTION DEBUG (DEBUG ASSERTION MACROS)
 * ===========================================================================
 * Makro assertion yang hanya aktif pada mode debug.
 */

#if DEBUG_ASSERT

/*
 * ASSERT
 * ------
 * Assertion dengan debug aktif.
 */
#define ASSERT(expr) \
    do { \
        if (!(expr)) { \
            kernel_panic_fungsi(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                                __func__, "Assertion gagal: " #expr); \
        } \
    } while (0)

/*
 * ASSERT_MSG
 * ----------
 * Assertion dengan pesan.
 */
#define ASSERT_MSG(expr, msg) \
    do { \
        if (!(expr)) { \
            kernel_panic_fungsi(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                                __func__, "Assertion gagal: %s", msg); \
        } \
    } while (0)

/*
 * ASSERT_PTR
 * ----------
 * Assertion pointer tidak NULL.
 */
#define ASSERT_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            kernel_panic_fungsi(PANIC_KODE_NULL_POINTER, __FILE__, __LINE__, \
                                __func__, "Pointer NULL: " #ptr); \
        } \
    } while (0)

/*
 * ASSERT_RANGE
 * ------------
 * Assertion nilai dalam rentang.
 */
#define ASSERT_RANGE(val, min_val, maks_val) \
    do { \
        if ((val) < (min_val) || (val) > (maks_val)) { \
            kernel_panic_fungsi(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                                __func__, \
                                "Nilai di luar rentang: %ld (valid: %ld-%ld)", \
                                (long)(val), (long)(min_val), (long)(maks_val)); \
        } \
    } while (0)

/*
 * ASSERT_EQ
 * ---------
 * Assertion equality.
 */
#define ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            kernel_panic_fungsi(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                                __func__, \
                                "Assertion gagal: %s != %s", #a, #b); \
        } \
    } while (0)

/*
 * ASSERT_NE
 * ---------
 * Assertion not equal.
 */
#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            kernel_panic_fungsi(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                                __func__, \
                                "Assertion gagal: %s == %s", #a, #b); \
        } \
    } while (0)

/*
 * ASSERT_LT
 * ---------
 * Assertion less than.
 */
#define ASSERT_LT(a, b) \
    do { \
        if (!((a) < (b))) { \
            kernel_panic_fungsi(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                                __func__, \
                                "Assertion gagal: %s >= %s", #a, #b); \
        } \
    } while (0)

/*
 * ASSERT_LE
 * ---------
 * Assertion less than or equal.
 */
#define ASSERT_LE(a, b) \
    do { \
        if (!((a) <= (b))) { \
            kernel_panic_fungsi(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                                __func__, \
                                "Assertion gagal: %s > %s", #a, #b); \
        } \
    } while (0)

/*
 * ASSERT_GT
 * ---------
 * Assertion greater than.
 */
#define ASSERT_GT(a, b) \
    do { \
        if (!((a) > (b))) { \
            kernel_panic_fungsi(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                                __func__, \
                                "Assertion gagal: %s <= %s", #a, #b); \
        } \
    } while (0)

/*
 * ASSERT_GE
 * ---------
 * Assertion greater than or equal.
 */
#define ASSERT_GE(a, b) \
    do { \
        if (!((a) >= (b))) { \
            kernel_panic_fungsi(PANIC_KODE_ASSERT, __FILE__, __LINE__, \
                                __func__, \
                                "Assertion gagal: %s < %s", #a, #b); \
        } \
    } while (0)

#else /* DEBUG_ASSERT */

/* Assertion dinonaktifkan saat DEBUG_ASSERT=0 */
#define ASSERT(expr) TIDAK_DIGUNAKAN(expr)
#define ASSERT_MSG(expr, msg) TIDAK_DIGUNAKAN(expr)
#define ASSERT_PTR(ptr) TIDAK_DIGUNAKAN(ptr)
#define ASSERT_RANGE(val, min_val, maks_val) TIDAK_DIGUNAKAN(val)
#define ASSERT_EQ(a, b) do { TIDAK_DIGUNAKAN(a); TIDAK_DIGUNAKAN(b); } while (0)
#define ASSERT_NE(a, b) do { TIDAK_DIGUNAKAN(a); TIDAK_DIGUNAKAN(b); } while (0)
#define ASSERT_LT(a, b) do { TIDAK_DIGUNAKAN(a); TIDAK_DIGUNAKAN(b); } while (0)
#define ASSERT_LE(a, b) do { TIDAK_DIGUNAKAN(a); TIDAK_DIGUNAKAN(b); } while (0)
#define ASSERT_GT(a, b) do { TIDAK_DIGUNAKAN(a); TIDAK_DIGUNAKAN(b); } while (0)
#define ASSERT_GE(a, b) do { TIDAK_DIGUNAKAN(a); TIDAK_DIGUNAKAN(b); } while (0)

#endif /* DEBUG_ASSERT */

/*
 * ===========================================================================
 * DEKLARASI VARIABEL GLOBAL (GLOBAL VARIABLE DECLARATIONS)
 * ===========================================================================
 */

/* Flag panic aktif */
extern volatile int g_panic_aktif;

/* Jumlah panic yang terjadi */
extern volatile tak_bertanda32_t g_panic_hitung;

/* Informasi panic terakhir */
extern panic_info_t g_panic_info_terakhir;

/* Nested panic count */
extern volatile tak_bertanda32_t g_panic_nested;

#endif /* INTI_PANIC_H */
