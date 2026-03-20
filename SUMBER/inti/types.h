/*
 * PIGURA OS - TYPES.H
 * -------------------
 * Definisi tipe data dasar untuk kernel Pigura OS.
 *
 * Berkas ini mendefinisikan tipe-tipe primitif yang portabel dan konsisten
 * di seluruh arsitektur yang didukung (x86, x86_64, ARM, ARMv7).
 *
 * Semua tipe didefinisikan dengan ukuran eksplisit untuk memastikan
 * portabilitas dan prediktabilitas di berbagai platform.
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef INTI_TYPES_H
#define INTI_TYPES_H

/*
 * ============================================================================
 * TIPA INTEGER BERTANDA (SIGNED INTEGER TYPES)
 * ============================================================================
 * Tipe-tipe ini memiliki rentang nilai negatif hingga positif.
 * Prefiks 'tanda_' mengindikasikan tipe bertanda.
 */

/* Tipe 8-bit bertanda: -128 sampai 127 */
typedef signed char tanda8_t;

/* Tipe 16-bit bertanda: -32768 sampai 32767 */
typedef signed short tanda16_t;

/* Tipe 32-bit bertanda: -2147483648 sampai 2147483647 */
typedef signed int tanda32_t;

/* Tipe 64-bit bertanda: -9223372036854775808 sampai 9223372036854775807 */
typedef signed long long tanda64_t;

/*
 * ============================================================================
 * TIPA INTEGER TAK BERTANDA (UNSIGNED INTEGER TYPES)
 * ============================================================================
 * Tipe-tipe ini hanya memiliki rentang nilai positif dan nol.
 * Prefiks 'tak_bertanda_' mengindikasikan tipe tak bertanda.
 */

/* Tipe 8-bit tak bertanda: 0 sampai 255 */
typedef unsigned char tak_bertanda8_t;

/* Tipe 16-bit tak bertanda: 0 sampai 65535 */
typedef unsigned short tak_bertanda16_t;

/* Tipe 32-bit tak bertanda: 0 sampai 4294967295 */
typedef unsigned int tak_bertanda32_t;

/* Tipe 64-bit tak bertanda: 0 sampai 18446744073709551615 */
typedef unsigned long long tak_bertanda64_t;

/*
 * ============================================================================
 * ALIAS TIPA PORTABEL (PORTABLE TYPE ALIASES)
 * ============================================================================
 * Alias-alias ini menyediakan nama yang lebih ringkas dan konsisten
 * dengan konvensi C standar, namun tetap dalam bahasa Indonesia.
 */

/* Alias untuk ukuran byte (8-bit) */
typedef tak_bertanda8_t byte_t;
typedef tanda8_t byte_tanda_t;

/* Alias untuk ukuran word (16-bit) */
typedef tak_bertanda16_t kata_t;
typedef tanda16_t kata_tanda_t;

/* Alias untuk ukuran dword (32-bit) */
typedef tak_bertanda32_t dwita_t;
typedef tanda32_t dwita_tanda_t;

/* Alias untuk ukuran qword (64-bit) */
typedef tak_bertanda64_t kwita_t;
typedef tanda64_t kwita_tanda_t;

/*
 * ============================================================================
 * TIPA UKURAN DAN JARAK (SIZE AND OFFSET TYPES)
 * ============================================================================
 * Tipe-tipe ini digunakan untuk merepresentasikan ukuran memori,
 * jarak pointer, dan operasi terkait memori.
 */

/* Tipe untuk ukuran objek di memori (setara size_t) */
typedef unsigned long ukuran_t;

/* Tipe untuk jarak antara dua pointer (setara ptrdiff_t) */
typedef signed long jarak_t;

/* Tipe untuk alamat memori fisik (selalu 64-bit untuk portabilitas) */
typedef tak_bertanda64_t alamat_fisik_t;

/* Tipe untuk alamat memori virtual */
typedef tak_bertanda64_t alamat_virtual_t;

/* Tipe untuk nomor halaman memori */
typedef tak_bertanda64_t nomor_halaman_t;

/* Tipe untuk offset dalam halaman */
typedef tak_bertanda32_t offset_halaman_t;

/*
 * ============================================================================
 * TIPA PROSES (PROCESS TYPES)
 * ============================================================================
 * Tipe-tipe yang berkaitan dengan manajemen proses dan thread.
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

/*
 * ============================================================================
 * TIPA POINTER (POINTER TYPES)
 * ============================================================================
 * Definisi tipe pointer yang umum digunakan dalam kernel.
 */

/* Pointer generik ke data */
typedef void *ptr_t;

/* Pointer konstan ke data */
typedef const void *ptr_konstan_t;

/* Pointer ke fungsi tanpa argumen dan tanpa return value */
typedef void (*fungsi_ptr_t)(void);

/* Pointer ke fungsi dengan satu argumen pointer */
typedef void (*fungsi_ptr_arg_t)(void *);

/* Pointer ke fungsi handler interupsi */
typedef void (*handler_interupsi_t)(void);

/* Pointer ke fungsi system call */
typedef long (*syscall_fn_t)(long, long, long, long, long, long);

/*
 * ============================================================================
 * KONSTANTA UKURAN (SIZE CONSTANTS)
 * ============================================================================
 * Konstanta-konstanta yang mendefinisikan ukuran dasar dalam byte.
 */

/* Ukuran byte dalam bit */
#define BIT_PER_BYTE        8

/* Ukuran halaman standar: 4 KB */
#define UKURAN_HALAMAN      4096UL

/* Log2 ukuran halaman untuk operasi bitwise */
#define LOG2_UKURAN_HALAMAN 12

/* Mask untuk offset dalam halaman */
#define MASK_OFFSET_HALAMAN 0xFFFUL

/* Ukuran word dalam byte */
#define UKURAN_KATA         2

/* Ukuran dword dalam byte */
#define UKURAN_DWITA        4

/* Ukuran qword dalam byte */
#define UKURAN_KWITA        8

/*
 * ============================================================================
 * KONSTANTA ARSITEKTUR (ARCHITECTURE CONSTANTS)
 * ============================================================================
 * Konstanta-konstanta yang bergantung pada arsitektur target.
 */

#if defined(ARSITEKTUR_X86) || defined(ARSITEKTUR_ARM)
    /* Arsitektur 32-bit */
    #define LEBAR_BIT           32
    #define LEBAR_BIT_LONG      32
    typedef tak_bertanda32_t alamat_ptr_t;
    typedef tak_bertanda32_t register_t;
#elif defined(ARSITEKTUR_X86_64) || defined(ARSITEKTUR_ARM64)
    /* Arsitektur 64-bit */
    #define LEBAR_BIT           64
    #define LEBAR_BIT_LONG      64
    typedef tak_bertanda64_t alamat_ptr_t;
    typedef tak_bertanda64_t register_t;
#else
    /* Default: asumsikan 32-bit */
    #define LEBAR_BIT           32
    #define LEBAR_BIT_LONG      32
    typedef tak_bertanda32_t alamat_ptr_t;
    typedef tak_bertanda32_t register_t;
#endif

/*
 * ============================================================================
 * KONSTANTA NILAI MINIMUM DAN MAKSIMUM (MIN/MAX VALUES)
 * ============================================================================
 * Batas nilai untuk setiap tipe integer.
 */

/* Nilai minimum dan maksimum tipe 8-bit */
#define TANDA8_MIN      ((tanda8_t)-128)
#define TANDA8_MAX      ((tanda8_t)127)
#define TAK_BERTANDA8_MAX ((tak_bertanda8_t)255)

/* Nilai minimum dan maksimum tipe 16-bit */
#define TANDA16_MIN     ((tanda16_t)-32768)
#define TANDA16_MAX     ((tanda16_t)32767)
#define TAK_BERTANDA16_MAX ((tak_bertanda16_t)65535)

/* Nilai minimum dan maksimum tipe 32-bit */
#define TANDA32_MIN     ((tanda32_t)-2147483647L - 1L)
#define TANDA32_MAX     ((tanda32_t)2147483647L)
#define TAK_BERTANDA32_MAX ((tak_bertanda32_t)4294967295UL)

/* Nilai minimum dan maksimum tipe 64-bit */
#define TANDA64_MIN     ((tanda64_t)-9223372036854775807LL - 1LL)
#define TANDA64_MAX     ((tanda64_t)9223372036854775807LL)
#define TAK_BERTANDA64_MAX ((tak_bertanda64_t)18446744073709551615ULL)

/*
 * ============================================================================
 * NILAI NULL DAN KONSTANTA KHUSUS (NULL AND SPECIAL VALUES)
 * ============================================================================
 */

/* Definisi NULL untuk pointer kosong */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Nilai invalid untuk pointer */
#define PTR_INVALID     ((void *)0xFFFFFFFFUL)

/* Nilai invalid untuk alamat fisik */
#define ALAMAT_FISIK_INVALID ((alamat_fisik_t)-1)

/* Nilai invalid untuk PID */
#define PID_INVALID     ((pid_t)-1)

/* Nilai invalid untuk TID */
#define TID_INVALID     ((tid_t)-1)

/*
 * ============================================================================
 * MAKRO PENGECEKAN TIPE (TYPE CHECKING MACROS)
 * ============================================================================
 */

/* Cek apakah pointer NULL */
#define PTR_KOSONG(p)   ((p) == NULL)

/* Cek apakah pointer valid (tidak NULL) */
#define PTR_VALID(p)    ((p) != NULL)

/* Cek apakah alamat aligned ke ukuran tertentu */
#define ALAMAT_RATA(addr, align) (((alamat_ptr_t)(addr) & ((align) - 1)) == 0)

/* Ratakan alamat ke batas alignment berikutnya */
#define RATAKAN_ATAS(addr, align) \
    (((alamat_ptr_t)(addr) + (align) - 1) & ~((align) - 1))

/* Ratakan alamat ke batas alignment sebelumnya */
#define RATAKAN_BAWAH(addr, align) \
    ((alamat_ptr_t)(addr) & ~((align) - 1))

/*
 * ============================================================================
 * KONVERSI ALAMAT (ADDRESS CONVERSION)
 * ============================================================================
 */

/* Konversi alamat virtual ke fisik (placeholder, diimplementasi di paging.c) */
#define VIRTUAL_KE_FISIK(vaddr) ((alamat_fisik_t)(vaddr))

/* Konversi alamat fisik ke virtual (placeholder, diimplementasi di paging.c) */
#define FISIK_KE_VIRTUAL(paddr) ((void *)(paddr))

/* Dapatkan nomor halaman dari alamat */
#define NOMOR_HALAMAN(addr) ((nomor_halaman_t)((alamat_ptr_t)(addr) >> LOG2_UKURAN_HALAMAN))

/* Dapatkan offset dalam halaman dari alamat */
#define OFFSET_HALAMAN(addr) ((offset_halaman_t)((alamat_ptr_t)(addr) & MASK_OFFSET_HALAMAN))

/*
 * ============================================================================
 * STRUKTUR STATUS (STATUS STRUCTURES)
 * ============================================================================
 */

/* Enum status operasi kernel */
typedef enum {
    STATUS_BERHASIL = 0,        /* Operasi berhasil */
    STATUS_GAGAL = -1,          /* Operasi gagal secara umum */
    STATUS_MEMORI_HABIS = -2,   /* Kehabisan memori */
    STATUS_PARAM_INVALID = -3,  /* Parameter tidak valid */
    STATUS_TIDAK_DITEMUKAN = -4,/* Sumber daya tidak ditemukan */
    STATUS_AKSES_DITOLAK = -5,  /* Akses ditolak */
    STATUS_SUDAH_ADA = -6,      /* Sumber daya sudah ada */
    STATUS_BUSY = -7,           /* Sumber daya sedang sibuk */
    STATUS_TIMEOUT = -8,        /* Operasi timeout */
    STATUS_TIDAK_DUKUNG = -9    /* Operasi tidak didukung */
} status_t;

/* Enum status proses */
typedef enum {
    PROSES_STATUS_BELUM = 0,    /* Proses baru dibuat, belum siap */
    PROSES_STATUS_SIAP = 1,     /* Proses siap dijalankan */
    PROSES_STATUS_JALAN = 2,    /* Proses sedang berjalan */
    PROSES_STATUS_TUNGGU = 3,   /* Proses menunggu event */
    PROSES_STATUS_ZOMBIE = 4,   /* Proses sudah selesai, menunggu cleanup */
    PROSES_STATUS_STOP = 5      /* Proses dihentikan sementara */
} proses_status_t;

/* Enum prioritas proses */
typedef enum {
    PRIORITAS_RENDAH = 0,       /* Prioritas rendah */
    PRIORITAS_NORMAL = 1,       /* Prioritas normal */
    PRIORITAS_TINGGI = 2,       /* Prioritas tinggi */
    PRIORITAS_REALTIME = 3      /* Prioritas realtime */
} prioritas_t;

/*
 * ============================================================================
 * STRUKTUR BOOLEAN (BOOLEAN STRUCTURE)
 * ============================================================================
 */

/* Tipe boolean */
typedef enum {
    SALAH = 0,  /* False */
    BENAR = 1   /* True */
} bool_t;

/* Alias untuk kompatibilitas */
#ifndef TRUE
#define TRUE    BENAR
#endif

#ifndef FALSE
#define FALSE   SALAH
#endif

#endif /* INTI_TYPES_H */
