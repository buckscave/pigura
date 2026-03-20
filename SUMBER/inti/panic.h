/*
 * PIGURA OS - PANIC.H
 * -------------------
 * Definisi dan deklarasi fungsi kernel panic.
 *
 * Berkas ini berisi fungsi-fungsi untuk menangani kondisi error fatal
 * yang tidak dapat dipulihkan. Kernel panic akan menghentikan sistem
 * dan menampilkan informasi error untuk debugging.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef INTI_PANIC_H
#define INTI_PANIC_H

#include "types.h"
#include "konstanta.h"

/*
 * ============================================================================
 * KONSTANTA PANIC (PANIC CONSTANTS)
 * ============================================================================
 */

/* Kode error panic */
#define PANIC_KODE_TIDAK_DIKETAHUI     0x0000
#define PANIC_KODE_ASSERT              0x0001
#define PANIC_KODE_NULL_POINTER        0x0002
#define PANIC_KODE_OUT_OF_MEMORY       0x0003
#define PANIC_KODE_STACK_OVERFLOW      0x0004
#define PANIC_KODE_STACK_UNDERFLOW     0x0005
#define PANIC_KODE_PAGE_FAULT          0x0006
#define PANIC_KODE_GENERAL_PROTECTION  0x0007
#define PANIC_KODE_DOUBLE_FAULT        0x0008
#define PANIC_KODE_INVALID_TSS         0x0009
#define PANIC_KODE_SEGMENT_NOT_PRESENT 0x000A
#define PANIC_KODE_INVALID_OPCODE      0x000B
#define PANIC_KODE_DEVICE_NOT_AVAIL    0x000C
#define PANIC_KODE_ALIGNMENT_CHECK     0x000D
#define PANIC_KODE_MACHINE_CHECK       0x000E
#define PANIC_KODE_DIVIDE_ERROR        0x000F
#define PANIC_KODE_OVERFLOW            0x0010
#define PANIC_KODE_BOUND_RANGE         0x0011
#define PANIC_KODE_FPU_EXCEPTION       0x0012
#define PANIC_KODE_SIMD_EXCEPTION      0x0013
#define PANIC_KODE_BREAKPOINT          0x0014
#define PANIC_KODE_NMI                 0x0015
#define PANIC_KODE_INTERRUPT           0x0020
#define PANIC_KODE_SYSCALL_INVALID     0x0030
#define PANIC_KODE_HAL_ERROR           0x0040
#define PANIC_KODE_VFS_ERROR           0x0050
#define PANIC_KODE_DRIVER_ERROR        0x0060
#define PANIC_KODE_USER_DEFINED        0x1000

/* Panjang maksimum pesan panic */
#define PANIC_PESAN_MAX                256

/* Panjang maksimum nama file */
#define PANIC_FILE_MAX                 128

/*
 * ============================================================================
 * STRUKTUR DATA PANIC (PANIC DATA STRUCTURES)
 * ============================================================================
 */

/* Struktur informasi panic */
typedef struct {
    tak_bertanda16_t kode;              /* Kode error */
    tak_bertanda16_t baris;             /* Nomor baris */
    char file[PANIC_FILE_MAX];          /* Nama file */
    char pesan[PANIC_PESAN_MAX];        /* Pesan error */
    alamat_ptr_t alamat_fault;          /* Alamat yang menyebabkan fault */
    tak_bertanda32_t error_code;        /* Error code dari CPU */
    tak_bertanda32_t cr2;               /* Nilai CR2 untuk page fault */
    tak_bertanda32_t cr3;               /* Nilai CR3 */
    tak_bertanda32_t eflags;            /* Nilai EFLAGS */
    tak_bertanda32_t eax, ebx, ecx, edx; /* Nilai register */
    tak_bertanda32_t esi, edi, ebp, esp; /* Nilai register */
    tak_bertanda32_t cs, ds, es, fs, gs; /* Nilai segment */
    tak_bertanda32_t ss;                /* Stack segment */
    tak_bertanda32_t eip;               /* Instruction pointer */
    tak_bertanda32_t pid;               /* PID proses saat ini */
    tak_bertanda64_t timestamp;         /* Waktu panic */
} panic_info_t;

/* Struktur konteks register untuk exception */
typedef struct {
    tak_bertanda32_t gs, fs, es, ds;    /* Segment registers */
    tak_bertanda32_t edi, esi, ebp, esp; /* General registers */
    tak_bertanda32_t ebx, edx, ecx, eax; /* General registers */
    tak_bertanda32_t int_no, err_code;  /* Interrupt number, error code */
    tak_bertanda32_t eip, cs, eflags;   /* Instruction pointer, CS, flags */
    tak_bertanda32_t useresp, ss;       /* User stack pointer, SS */
} register_context_t;

/*
 * ============================================================================
 * DEKLARASI FUNGSI (FUNCTION DECLARATIONS)
 * ============================================================================
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
 *
 * Return: Tidak pernah return
 */
void kernel_panic(const char *file, int baris,
                  const char *format, ...) __attribute__((noreturn));

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
 *
 * Return: Tidak pernah return
 */
void kernel_panic_kode(tak_bertanda16_t kode, const char *file, int baris,
                       const char *format, ...) __attribute__((noreturn));

/*
 * kernel_panic_register
 * ---------------------
 * Kernel panic dengan informasi register lengkap.
 *
 * Parameter:
 *   info - Pointer ke struktur informasi panic
 *
 * Return: Tidak pernah return
 */
void kernel_panic_register(const panic_info_t *info) __attribute__((noreturn));

/*
 * kernel_panic_exception
 * ----------------------
 * Kernel panic dari exception handler.
 *
 * Parameter:
 *   ctx - Pointer ke konteks register saat exception
 *
 * Return: Tidak pernah return
 */
void kernel_panic_exception(const register_context_t *ctx)
    __attribute__((noreturn));

/*
 * kernel_halt
 * -----------
 * Hentikan sistem tanpa pesan error.
 * Digunakan untuk shutdown normal atau kondisi dimana
 * pesan error tidak diperlukan.
 *
 * Return: Tidak pernah return
 */
void kernel_halt(void) __attribute__((noreturn));

/*
 * kernel_reboot
 * -------------
 * Reboot sistem.
 *
 * Parameter:
 *   force - Jika tidak nol, lakukan hard reset
 *
 * Return: Tidak pernah return jika berhasil
 */
void kernel_reboot(int force) __attribute__((noreturn));

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
 *   esp   - Stack pointer
 *   depth - Jumlah dword yang ditampilkan
 */
void panic_dump_stack(tak_bertanda32_t esp, int depth);

/*
 * panic_dump_memory
 * -----------------
 * Tampilkan dump memory.
 *
 * Parameter:
 *   addr  - Alamat awal
 *   len   - Panjang dalam byte
 */
void panic_dump_memory(void *addr, ukuran_t len);

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
 * ============================================================================
 * MAKRO PANIC (PANIC MACROS)
 * ============================================================================
 */

/*
 * PANIC
 * -----
 * Macro untuk memanggil kernel_panic dengan otomatis mengisi
 * nama file dan nomor baris.
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
    kernel_panic(__FILE__, __LINE__, "Fungsi belum diimplementasi: %s", \
                 __func__)

/*
 * PANIC_INVALID_STATE
 * -------------------
 * Panic untuk state tidak valid.
 */
#define PANIC_INVALID_STATE(fmt, ...) \
    kernel_panic(__FILE__, __LINE__, "State tidak valid: " fmt, \
                 ##__VA_ARGS__)

/*
 * PANIC_BOUNDS
 * ------------
 * Panic untuk index di luar batas.
 */
#define PANIC_BOUNDS(idx, min, maks) \
    kernel_panic(__FILE__, __LINE__, \
                 "Index di luar batas: %ld (valid: %ld-%ld)", \
                 (long)(idx), (long)(min), (long)(maks))

/*
 * ============================================================================
 * MAKRO ASSERTION (ASSERTION MACROS)
 * ============================================================================
 */

/*
 * BUILD_ASSERT
 * ------------
 * Assertion pada compile-time.
 */
#define BUILD_ASSERT(cond) \
    typedef char __build_assert_##__LINE__[(cond) ? 1 : -1] \
        __attribute__((unused))

/*
 * BUILD_ASSERT_MSG
 * ----------------
 * Assertion pada compile-time dengan pesan.
 */
#define BUILD_ASSERT_MSG(cond, msg) \
    _Static_assert(cond, msg)

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
 * ============================================================================
 * DEKLARASI VARIABEL GLOBAL (GLOBAL VARIABLE DECLARATIONS)
 * ============================================================================
 */

/* Flag panic aktif */
extern volatile int g_panic_aktif;

/* Jumlah panic yang terjadi */
extern volatile tak_bertanda32_t g_panic_hitung;

/* Informasi panic terakhir */
extern panic_info_t g_panic_info_terakhir;

#endif /* INTI_PANIC_H */
