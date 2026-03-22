/*
 * PIGURA OS - TYPES.H
 * ====================
 * Definisi tipe data dasar portabel untuk kernel Pigura OS.
 *
 * Berkas ini mendefinisikan tipe-tipe primitif yang portabel dan konsisten
 * di seluruh arsitektur yang didukung (x86, x86_64, ARM, ARM64).
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Semua tipe didefinisikan secara eksplisit
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#ifndef INTI_TYPES_H
#define INTI_TYPES_H

/*
 * ===========================================================================
 * DETEKSI ARSITEKTUR (ARCHITECTURE DETECTION)
 * ===========================================================================
 * Deteksi otomatis arsitektur target berdasarkan compiler predefined macros.
 * Arsitektur menentukan ukuran pointer dan tipe data yang bergantung platform.
 */

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    #define PIGURA_ARSITEKTUR_64BIT 1
    #define PIGURA_LEBAR_BIT 64
#elif defined(__i386__) || defined(_M_IX86) || defined(__arm__) || \
      defined(_M_ARM)
    #define PIGURA_ARSITEKTUR_32BIT 1
    #define PIGURA_LEBAR_BIT 32
#else
    #error "Arsitektur tidak didukung: prosesor tidak dikenali"
#endif

/*
 * ===========================================================================
 * TIPE INTEGER BERTANDA (SIGNED INTEGER TYPES)
 * ===========================================================================
 * Tipe-tipe integer bertanda dengan ukuran tetap dan portabel.
 * Prefiks 'tanda_' mengindikasikan tipe bertanda (signed).
 */

/* Tipe 8-bit bertanda: rentang -128 sampai 127 */
typedef signed char tanda8_t;

/* Tipe 16-bit bertanda: rentang -32768 sampai 32767 */
typedef signed short tanda16_t;

/* Tipe 32-bit bertanda: rentang -2147483648 sampai 2147483647 */
typedef signed int tanda32_t;

/* Tipe 64-bit bertanda: rentang -9223372036854775808 sampai 9223372036854775807 */
typedef signed long long tanda64_t;

/*
 * ===========================================================================
 * TIPE INTEGER TAK BERTANDA (UNSIGNED INTEGER TYPES)
 * ===========================================================================
 * Tipe-tipe integer tak bertanda dengan ukuran tetap dan portabel.
 * Prefiks 'tak_bertanda_' mengindikasikan tipe tak bertanda (unsigned).
 */

/* Tipe 8-bit tak bertanda: rentang 0 sampai 255 */
typedef unsigned char tak_bertanda8_t;

/* Tipe 16-bit tak bertanda: rentang 0 sampai 65535 */
typedef unsigned short tak_bertanda16_t;

/* Tipe 32-bit tak bertanda: rentang 0 sampai 4294967295 */
typedef unsigned int tak_bertanda32_t;

/* Tipe 64-bit tak bertanda: rentang 0 sampai 18446744073709551615 */
typedef unsigned long long tak_bertanda64_t;

/*
 * ===========================================================================
 * TIPE UKURAN DAN POINTER (SIZE AND POINTER TYPES)
 * ===========================================================================
 * Tipe-tipe yang ukurannya bergantung pada arsitektur target.
 * Pada sistem 32-bit: 4 byte, pada sistem 64-bit: 8 byte.
 */

/* Tipe untuk ukuran objek di memori (setara size_t) */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    typedef tak_bertanda64_t ukuran_t;
    typedef tanda64_t jarak_t;
#else
    typedef tak_bertanda32_t ukuran_t;
    typedef tanda32_t jarak_t;
#endif

/* Tipe untuk jarak antara dua pointer (setara ptrdiff_t) */
typedef jarak_t jarak_ptr_t;

/* Tipe untuk alamat memori virtual (pointer-sized) */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    typedef tak_bertanda64_t alamat_virtual_t;
#else
    typedef tak_bertanda32_t alamat_virtual_t;
#endif

/* Tipe untuk alamat memori fisik (selalu 64-bit untuk portabilitas) */
typedef tak_bertanda64_t alamat_fisik_t;

/* Tipe untuk nomor halaman memori */
typedef tak_bertanda64_t nomor_halaman_t;

/* Tipe untuk offset dalam halaman (selalu 32-bit, maksimal 4KB) */
typedef tak_bertanda32_t offset_halaman_t;

/* Tipe untuk menyimpan pointer sebagai integer */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    typedef tak_bertanda64_t uintptr_t;
    typedef tanda64_t intptr_t;
#else
    typedef tak_bertanda32_t uintptr_t;
    typedef tanda32_t intptr_t;
#endif

/*
 * ===========================================================================
 * TIPE INTEGER PORTABEL (PORTABLE INTEGER TYPES)
 * ===========================================================================
 * Alias untuk kompatibilitas dengan konvensi penamaan standar.
 */

/* Alias untuk byte (8-bit) */
typedef tak_bertanda8_t byte_t;
typedef tanda8_t byte_tanda_t;

/* Alias untuk word (16-bit) */
typedef tak_bertanda16_t kata_t;
typedef tanda16_t kata_tanda_t;

/* Alias untuk dword (32-bit) */
typedef tak_bertanda32_t dwita_t;
typedef tanda32_t dwita_tanda_t;

/* Alias untuk qword (64-bit) */
typedef tak_bertanda64_t kwita_t;
typedef tanda64_t kwita_tanda_t;

/* Tipe integer terbesar yang tersedia */
typedef tanda64_t tanda_terbesar_t;
typedef tak_bertanda64_t tak_bertanda_terbesar_t;

/*
 * ===========================================================================
 * TIPE PROSES DAN SISTEM (PROCESS AND SYSTEM TYPES)
 * ===========================================================================
 * Tipe-tipe yang berkaitan dengan manajemen proses, thread, dan sistem.
 */

/* Tipe untuk Process ID */
typedef tak_bertanda32_t pid_t;

/* Tipe untuk Thread ID */
typedef tak_bertanda32_t tid_t;

/* Tipe untuk Group ID */
typedef tak_bertanda32_t gid_t;

/* Tipe untuk User ID */
typedef tak_bertanda32_t uid_t;

/* Tipe untuk mode file dan permission */
typedef tak_bertanda16_t mode_t;

/* Tipe untuk device number */
typedef tak_bertanda32_t dev_t;

/* Tipe untuk inode number */
typedef tak_bertanda64_t ino_t;

/* Tipe untuk file offset */
typedef tanda64_t off_t;

/* Tipe untuk waktu (detik sejak epoch) */
typedef tanda64_t waktu_t;

/* Tipe untuk clock ticks */
typedef tak_bertanda64_t clock_t;

/* Tipe untuk milliseconds */
typedef tak_bertanda64_t mdetik_t;

/* Tipe untuk microseconds */
typedef tak_bertanda64_t udetik_t;

/* Tipe untuk nanoseconds */
typedef tak_bertanda64_t ndetik_t;

/*
 * ===========================================================================
 * TIPE POINTER (POINTER TYPES)
 * ===========================================================================
 * Definisi tipe pointer yang umum digunakan dalam kernel.
 */

/* Pointer generik ke data */
typedef void *ptr_t;

/* Pointer konstan ke data */
typedef const void *ptr_konstan_t;

/* Pointer ke fungsi tanpa argumen dan tanpa return value */
typedef void (*fungsi_void_t)(void);

/* Pointer ke fungsi dengan satu argumen pointer */
typedef void (*fungsi_ptr_t)(void *);

/* Pointer ke fungsi dengan dua argumen pointer */
typedef void (*fungsi_ptr2_t)(void *, void *);

/* Pointer ke fungsi handler interupsi */
typedef void (*handler_interupsi_t)(void);

/* Pointer ke fungsi system call */
typedef tanda64_t (*syscall_fn_t)(tanda64_t, tanda64_t, tanda64_t,
                                   tanda64_t, tanda64_t, tanda64_t);

/* Pointer ke fungsi perbandingan */
typedef int (*fungsi_banding_t)(ptr_konstan_t, ptr_konstan_t);

/* Pointer ke fungsi alokasi memori */
typedef ptr_t (*fungsi_alokasi_t)(ukuran_t);

/* Pointer ke fungsi dealokasi memori */
typedef void (*fungsi_bebaskan_t)(ptr_t);

/*
 * ===========================================================================
 * TIPE BOOLEAN (BOOLEAN TYPE)
 * ===========================================================================
 * Tipe boolean yang kompatibel dengan C90.
 */

typedef enum {
    SALAH = 0,
    BENAR = 1
} bool_t;

/*
 * ===========================================================================
 * TIPE STATUS OPERASI (OPERATION STATUS TYPE)
 * ===========================================================================
 * Enum status operasi kernel dengan kode error yang jelas.
 */

typedef enum {
    /* Status berhasil */
    STATUS_BERHASIL = 0,

    /* Status error umum */
    STATUS_GAGAL = -1,
    STATUS_ERROR_TIDAK_DIKETAHUI = -2,

    /* Status error parameter */
    STATUS_PARAM_INVALID = -3,
    STATUS_PARAM_NULL = -4,
    STATUS_PARAM_JARAK = -5,
    STATUS_PARAM_UKURAN = -6,

    /* Status error memori */
    STATUS_MEMORI_HABIS = -10,
    STATUS_MEMORI_TIDAK_CUKUP = -11,
    STATUS_MEMORI_ALIGN = -12,
    STATUS_MEMORI_OVERLAY = -13,
    STATUS_MEMORI_CORRUPT = -14,

    /* Status error akses */
    STATUS_AKSES_DITOLAK = -20,
    STATUS_AKSES_INVALID = -21,
    STATUS_HAK_INVALID = -22,

    /* Status error sumber daya */
    STATUS_TIDAK_DITEMUKAN = -30,
    STATUS_SUDAH_ADA = -31,
    STATUS_BUSY = -32,
    STATUS_TIMEOUT = -33,
    STATUS_KOSONG = -34,
    STATUS_PENUH = -35,

    /* Status error I/O */
    STATUS_IO_ERROR = -40,
    STATUS_IO_TIMEOUT = -41,
    STATUS_IO_RESET = -42,

    /* Status error filesystem */
    STATUS_FS_TIDAK_DUKUNG = -50,
    STATUS_FS_CORRUPT = -51,
    STATUS_FS_READONLY = -52,
    STATUS_FS_PENUH = -53,
    STATUS_FS_TIDAK_ADA = -54,

    /* Status error proses */
    STATUS_PROSES_TIDAK_ADA = -60,
    STATUS_PROSES_ZOMBIE = -61,
    STATUS_PROSES_STOP = -62,
    STATUS_PROSES_RUNNING = -63,

    /* Status error thread */
    STATUS_THREAD_TIDAK_ADA = -70,
    STATUS_THREAD_EXIT = -71,

    /* Status error jaringan */
    STATUS_NET_TIDAK_ADA = -80,
    STATUS_NET_TIMEOUT = -81,
    STATUS_NET_RESET = -82,
    STATUS_NET_REFUSED = -83,

    /* Status tidak terimplementasi */
    STATUS_TIDAK_DUKUNG = -100,
    STATUS_BELUM_IMPLEMENTASI = -101
} status_t;

/*
 * ===========================================================================
 * TIPE STATUS PROSES (PROCESS STATUS TYPE)
 * ===========================================================================
 * Enum status proses untuk manajemen scheduler.
 */

typedef enum {
    PROSES_STATUS_INVALID = 0,
    PROSES_STATUS_BELUM = 1,
    PROSES_STATUS_SIAP = 2,
    PROSES_STATUS_JALAN = 3,
    PROSES_STATUS_TUNGGU = 4,
    PROSES_STATUS_ZOMBIE = 5,
    PROSES_STATUS_STOP = 6,
    PROSES_STATUS_BLOCK = 7
} proses_status_t;

/*
 * ===========================================================================
 * TIPE STATUS THREAD (THREAD STATUS TYPE)
 * ===========================================================================
 * Enum status thread untuk manajemen scheduler.
 */

typedef enum {
    THREAD_STATUS_INVALID = 0,
    THREAD_STATUS_BELUM = 1,
    THREAD_STATUS_SIAP = 2,
    THREAD_STATUS_JALAN = 3,
    THREAD_STATUS_TUNGGU = 4,
    THREAD_STATUS_STOP = 5,
    THREAD_STATUS_BLOCK = 6,
    THREAD_STATUS_SLEEP = 7
} thread_status_t;

/*
 * ===========================================================================
 * TIPE PRIORITAS (PRIORITY TYPE)
 * ===========================================================================
 * Enum prioritas proses dan thread.
 */

typedef enum {
    PRIORITAS_INVALID = 0,
    PRIORITAS_IDLE = 1,
    PRIORITAS_RENDAH = 2,
    PRIORITAS_NORMAL = 3,
    PRIORITAS_TINGGI = 4,
    PRIORITAS_REALTIME = 5,
    PRIORITAS_KRITIS = 6
} prioritas_t;

/*
 * ===========================================================================
 * TIPE STATUS MEMORI (MEMORY STATUS TYPE)
 * ===========================================================================
 * Enum status region memori.
 */

typedef enum {
    MEMORI_STATUS_INVALID = 0,
    MEMORI_STATUS_BEbas = 1,
    MEMORI_STATUS_DIPAKAI = 2,
    MEMORI_STATUS_RESERVE = 3,
    MEMORI_STATUS_LOCK = 4,
    MEMORI_STATUS_SHARED = 5
} memori_status_t;

/*
 * ===========================================================================
 * NILAI NULL (NULL VALUE)
 * ===========================================================================
 * Definisi NULL yang kompatibel dengan C90.
 */

#ifndef NULL
#define NULL ((void *)0)
#endif

/*
 * ===========================================================================
 * KONSTANTA UKURAN DASAR (BASIC SIZE CONSTANTS)
 * ===========================================================================
 * Konstanta-konstanta ukuran dasar yang tidak bergantung konfigurasi.
 */

/* Ukuran byte dalam bit */
#define BIT_PER_BYTE 8

/* Ukuran byte dalam bit (alias) */
#define UKURAN_BYTE_BIT 8

/* Ukuran short dalam byte */
#define UKURAN_SHORT_BYTE 2

/* Ukuran int dalam byte */
#define UKURAN_INT_BYTE 4

/* Ukuran long dalam byte (platform dependent) */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    #define UKURAN_LONG_BYTE 8
#else
    #define UKURAN_LONG_BYTE 4
#endif

/* Ukuran long long dalam byte */
#define UKURAN_LLONG_BYTE 8

/* Ukuran pointer dalam byte */
#if defined(PIGURA_ARSITEKTUR_64BIT)
    #define UKURAN_PTR_BYTE 8
#else
    #define UKURAN_PTR_BYTE 4
#endif

/* Ukuran halaman standar: 4 KB */
#define UKURAN_HALAMAN 4096UL

/* Log2 ukuran halaman untuk operasi bitwise */
#define LOG2_UKURAN_HALAMAN 12

/* Mask untuk offset dalam halaman */
#define MASK_OFFSET_HALAMAN 0xFFFUL

/* Ukuran halaman besar: 2 MB */
#define UKURAN_HALAMAN_BESAR (2UL * 1024UL * 1024UL)

/* Ukuran halaman raksasa: 1 GB */
#define UKURAN_HALAMAN_RAKSASA (1UL * 1024UL * 1024UL * 1024UL)

/*
 * ===========================================================================
 * NILAI MINIMUM DAN MAKSIMUM (MIN/MAX VALUES)
 * ===========================================================================
 * Batas nilai untuk setiap tipe integer.
 */

/* Nilai minimum dan maksimum tipe 8-bit */
#define TANDA8_MIN ((tanda8_t)(-128))
#define TANDA8_MAX ((tanda8_t)(127))
#define TAK_BERTANDA8_MAX ((tak_bertanda8_t)(255U))
#define TAK_BERTANDA8_MIN ((tak_bertanda8_t)(0U))

/* Nilai minimum dan maksimum tipe 16-bit */
#define TANDA16_MIN ((tanda16_t)(-32768))
#define TANDA16_MAX ((tanda16_t)(32767))
#define TAK_BERTANDA16_MAX ((tak_bertanda16_t)(65535U))
#define TAK_BERTANDA16_MIN ((tak_bertanda16_t)(0U))

/* Nilai minimum dan maksimum tipe 32-bit */
#define TANDA32_MIN ((tanda32_t)(-2147483647L - 1L))
#define TANDA32_MAX ((tanda32_t)(2147483647L))
#define TAK_BERTANDA32_MAX ((tak_bertanda32_t)(4294967295UL))
#define TAK_BERTANDA32_MIN ((tak_bertanda32_t)(0UL))

/* Nilai minimum dan maksimum tipe 64-bit */
#define TANDA64_MIN ((tanda64_t)(-9223372036854775807LL - 1LL))
#define TANDA64_MAX ((tanda64_t)(9223372036854775807LL))
#define TAK_BERTANDA64_MAX ((tak_bertanda64_t)(18446744073709551615ULL))
#define TAK_BERTANDA64_MIN ((tak_bertanda64_t)(0ULL))

/*
 * ===========================================================================
 * NILAI INVALID (INVALID VALUES)
 * ===========================================================================
 * Nilai-nilai invalid untuk pengecekan error.
 */

/* Nilai invalid untuk pointer */
#define PTR_INVALID ((void *)(uintptr_t)(-1))

/* Nilai invalid untuk alamat fisik */
#define ALAMAT_FISIK_INVALID ((alamat_fisik_t)(-1LL))

/* Nilai invalid untuk alamat virtual */
#define ALAMAT_VIRTUAL_INVALID ((alamat_virtual_t)(-1ULL))

/* Nilai invalid untuk PID */
#define PID_INVALID ((pid_t)(-1))

/* Nilai invalid untuk TID */
#define TID_INVALID ((tid_t)(-1))

/* Nilai invalid untuk UID */
#define UID_INVALID ((uid_t)(-1))

/* Nilai invalid untuk GID */
#define GID_INVALID ((gid_t)(-1))

/* Nilai invalid untuk ukuran */
#define UKURAN_INVALID ((ukuran_t)(-1))

/* Nilai invalid untuk offset */
#define OFFSET_INVALID ((off_t)(-1LL))

/* Nilai invalid untuk handle */
#define HANDLE_INVALID ((tak_bertanda32_t)(-1))

/* Nilai invalid untuk index */
#define INDEX_INVALID ((tak_bertanda32_t)(-1))

/*
 * ===========================================================================
 * ALIAS KOMPATIBILITAS (COMPATIBILITY ALIASES)
 * ===========================================================================
 * Alias untuk kompatibilitas dengan kode yang sudah ada.
 */

#ifndef TRUE
#define TRUE BENAR
#endif

#ifndef FALSE
#define FALSE SALAH
#endif

/* Alias untuk byte order */
#define BYTE_ORDER_LITTLE 1
#define BYTE_ORDER_BIG 2

/* Deteksi byte order dari predefined macros */
#if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define BYTE_ORDER BYTE_ORDER_LITTLE
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define BYTE_ORDER BYTE_ORDER_BIG
    #endif
#elif defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || \
      defined(_M_X64) || defined(__arm__) || defined(__aarch64__)
    #define BYTE_ORDER BYTE_ORDER_LITTLE
#else
    #define BYTE_ORDER BYTE_ORDER_LITTLE
#endif

/*
 * ===========================================================================
 * MAKRO UKURAN STRUCT (STRUCT SIZE MACROS)
 * ===========================================================================
 * Makro untuk mendapatkan ukuran array dan offset member.
 */

/* Mendapatkan ukuran array dalam elemen */
#define ARRAY_UKURAN(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Mendapatkan offset member dalam struct */
#define OFFSET_OF(tipe, member) ((ukuran_t) &((tipe *)0)->member)

/* Mendapatkan struct dari pointer member */
#define CONTAINER_OF(ptr, tipe, member) \
    ((tipe *)((char *)(ptr) - OFFSET_OF(tipe, member)))

/*
 * ===========================================================================
 * MAKRO ALIGNMENT (ALIGNMENT MACROS)
 * ===========================================================================
 * Makro untuk operasi alignment memori.
 */

/* Ratakan ke atas ke batas alignment */
#define RATAKAN_ATAS(val, align) \
    (((val) + ((align) - 1)) & ~((align) - 1))

/* Ratakan ke bawah ke batas alignment */
#define RATAKAN_BAWAH(val, align) \
    ((val) & ~((align) - 1))

/* Cek apakah sudah aligned */
#define SUDAH_RATA(val, align) (((val) & ((align) - 1)) == 0)

/* Dapatkan offset dari alignment */
#define OFFSET_RATA(val, align) ((val) & ((align) - 1))

/* Alignment standar */
#define ALIGN_4BYTE 4
#define ALIGN_8BYTE 8
#define ALIGN_16BYTE 16
#define ALIGN_32BYTE 32
#define ALIGN_64BYTE 64
#define ALIGN_HALAMAN UKURAN_HALAMAN

/*
 * ===========================================================================
 * MAKRO BOUNDARY CHECK (BOUNDARY CHECK MACROS)
 * ===========================================================================
 * Makro untuk pengecekan batas yang aman.
 */

/* Cek apakah nilai dalam rentang [min, maks] */
#define DALAM_RENTANG(val, min, maks) \
    (((val) >= (min)) && ((val) <= (maks)))

/* Cek apakah nilai dalam rentang [min, maks) */
#define DALAM_RENTANG_TERBUKA(val, min, maks) \
    (((val) >= (min)) && ((val) < (maks)))

/* Cek apakah index valid dalam array */
#define INDEX_VALID(idx, arr) \
    DALAM_RENTANG_TERBUKA(idx, 0, ARRAY_UKURAN(arr))

/* Cek apakah pointer aligned */
#define PTR_ALIGNED(ptr, align) SUDAH_RATA((uintptr_t)(ptr), align)

/*
 * ===========================================================================
 * MAKRO BITWISE (BITWISE MACROS)
 * ===========================================================================
 * Makro untuk operasi bitwise yang aman.
 */

/* Set bit */
#define BIT_SET(reg, bit) ((reg) |= (1UL << (bit)))

/* Clear bit */
#define BIT_CLEAR(reg, bit) ((reg) &= ~(1UL << (bit)))

/* Toggle bit */
#define BIT_TOGGLE(reg, bit) ((reg) ^= (1UL << (bit)))

/* Cek apakah bit set */
#define BIT_TEST(reg, bit) (((reg) >> (bit)) & 1UL)

/* Set beberapa bit */
#define BITS_SET(reg, mask) ((reg) |= (mask))

/* Clear beberapa bit */
#define BITS_CLEAR(reg, mask) ((reg) &= ~(mask))

/* Dapatkan nilai bit dalam rentang */
#define BITS_GET(reg, mulai, lebar) \
    (((reg) >> (mulai)) & ((1UL << (lebar)) - 1UL))

/* Set nilai bit dalam rentang */
#define BITS_SET_NILAI(reg, mulai, lebar, nilai) \
    do { \
        tak_bertanda32_t _mask = ((1UL << (lebar)) - 1UL); \
        (reg) = ((reg) & ~(_mask << (mulai))) | \
                (((nilai) & _mask) << (mulai)); \
    } while (0)

/* Mask untuk bit rendah */
#define MASK_BAWAH(n) ((1UL << (n)) - 1UL)

/* Mask untuk bit tinggi */
#define MASK_ATAS(n) (~((1UL << (n)) - 1UL))

/* Byte low dan high */
#define BYTE_RENDAH(w) ((tak_bertanda8_t)((w) & 0xFFU))
#define BYTE_TINGGI(w) ((tak_bertanda8_t)(((w) >> 8) & 0xFFU))

/* Word low dan high */
#define KATA_RENDAH(d) ((tak_bertanda16_t)((d) & 0xFFFFU))
#define KATA_TINGGI(d) ((tak_bertanda16_t)(((d) >> 16) & 0xFFFFU))

/* Dword low dan high */
#define DWITA_RENDAH(q) ((tak_bertanda32_t)((q) & 0xFFFFFFFFUL))
#define DWITA_TINGGI(q) ((tak_bertanda32_t)(((q) >> 32) & 0xFFFFFFFFUL))

/* Gabungkan byte menjadi word */
#define GABUNG_KATA(lo, hi) \
    ((tak_bertanda16_t)(((hi) << 8) | (lo)))

/* Gabungkan word menjadi dword */
#define GABUNG_DWITA(lo, hi) \
    ((tak_bertanda32_t)(((hi) << 16) | (lo)))

/* Gabungkan dword menjadi qword */
#define GABUNG_KWITA(lo, hi) \
    ((tak_bertanda64_t)(((tak_bertanda64_t)(hi) << 32) | (lo)))

/*
 * ===========================================================================
 * MAKRO MINIMUM DAN MAKSIMUM (MIN/MAX MACROS)
 * ===========================================================================
 * Makro untuk mendapatkan nilai minimum dan maksimum.
 */

/* Nilai minimum dua bilangan */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* Nilai maksimum dua bilangan */
#define MAKS(a, b) (((a) > (b)) ? (a) : (b))

/* Batasi nilai dalam rentang */
#define BATAS(val, min, maks) (MIN(MAKS(val, min), maks))

/* Tukar dua nilai */
#define TUKAR(a, b, tipe) \
    do { \
        tipe _tmp = (a); \
        (a) = (b); \
        (b) = _tmp; \
    } while (0)

/*
 * ===========================================================================
 * MAKRO STRING (STRING MACROS)
 * ===========================================================================
 * Makro untuk konversi string.
 */

/* Konversi ke string */
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/*
 * ===========================================================================
 * STATIC ASSERTION (COMPILE-TIME ASSERTION)
 * ===========================================================================
 * Static assertion yang kompatibel dengan C90.
 */

/* Static assertion dengan nama unik */
#define STATIC_ASSERT(cond, nama) \
    typedef char static_assert_##nama[(cond) ? 1 : -1]

/* Static assertion untuk ukuran tipe */
#define STATIC_ASSERT_SIZE(tipe, ukuran) \
    STATIC_ASSERT(sizeof(tipe) == (ukuran), tipe##_ukuran_##ukuran)

/* Verifikasi ukuran tipe dasar */
STATIC_ASSERT_SIZE(tanda8_t, 1);
STATIC_ASSERT_SIZE(tanda16_t, 2);
STATIC_ASSERT_SIZE(tanda32_t, 4);
STATIC_ASSERT_SIZE(tanda64_t, 8);
STATIC_ASSERT_SIZE(tak_bertanda8_t, 1);
STATIC_ASSERT_SIZE(tak_bertanda16_t, 2);
STATIC_ASSERT_SIZE(tak_bertanda32_t, 4);
STATIC_ASSERT_SIZE(tak_bertanda64_t, 8);

#endif /* INTI_TYPES_H */
