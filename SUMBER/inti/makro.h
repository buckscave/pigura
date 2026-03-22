/*
 * PIGURA OS - MAKRO.H
 * ====================
 * Definisi makro utilitas untuk kernel Pigura OS.
 *
 * Berkas ini berisi makro-makro umum yang digunakan di seluruh kernel.
 * Makro dasar (alignment, bitwise, min/max) ada di types.h.
 *
 * KEPATUHAN STANDAR:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Tidak menggunakan fitur C99/C11
 * - Tidak menggunakan 'static inline'
 *
 * Versi: 2.0
 * Tanggal: 2025
 */

#ifndef INTI_MAKRO_H
#define INTI_MAKRO_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "types.h"

/*
 * ===========================================================================
 * CATATAN PENTING
 * ===========================================================================
 * Makro alignment, bitwise, min/max sudah didefinisikan di types.h.
 * File ini hanya berisi makro tambahan yang spesifik untuk operasi kernel.
 */

/*
 * ===========================================================================
 * MAKRO KONTROL ALUR (FLOW CONTROL MACROS)
 * ===========================================================================
 */

/* Barrier memori (prevent reordering) */
#define BARRIER_MEMORI() __asm__ __volatile__("" ::: "memory")

/* Compiler hint untuk kondisi yang sering benar */
#define KEMUNGKINAN(x) __builtin_expect(!!(x), 1)

/* Compiler hint untuk kondisi yang jarang benar */
#define JARANG(x) __builtin_expect(!!(x), 0)

/* Prefetch memory */
#define PREFETCH(addr) __builtin_prefetch(addr, 0, 3)

/*
 * ===========================================================================
 * MAKRO ERROR HANDLING (ERROR HANDLING MACROS)
 * ===========================================================================
 */

/* Cek dan return jika error */
#define CEK_ERROR(expr) \
    do { \
        status_t _status_ce = (expr); \
        if (_status_ce != STATUS_BERHASIL) { \
            return _status_ce; \
        } \
    } while (0)

/* Cek dan goto jika error */
#define CEK_ERROR_GOTO(expr, label) \
    do { \
        status_t _status_ceg = (expr); \
        if (_status_ceg != STATUS_BERHASIL) { \
            goto label; \
        } \
    } while (0)

/* Cek pointer dan return error */
#define CEK_PTR(ptr) \
    do { \
        if ((ptr) == NULL) { \
            return STATUS_PARAM_NULL; \
        } \
    } while (0)

/* Cek pointer dan goto error */
#define CEK_PTR_GOTO(ptr, label) \
    do { \
        if ((ptr) == NULL) { \
            goto label; \
        } \
    } while (0)

/* Cek alokasi memori */
#define CEK_ALOKASI(ptr) \
    do { \
        if ((ptr) == NULL) { \
            return STATUS_MEMORI_HABIS; \
        } \
    } while (0)

/*
 * ===========================================================================
 * MAKRO LINKED LIST (LINKED LIST MACROS)
 * ===========================================================================
 */

/* Struktur list head */
struct list_head {
    struct list_head *berikutnya;
    struct list_head *sebelumnya;
};

/* Tipe untuk list head */
typedef struct list_head list_head_t;

/* Inisialisasi list head statis */
#define LIST_HEAD_INIT(nama) { &(nama), &(nama) }

/* Deklarasi list head statis */
#define LIST_HEAD(nama) \
    list_head_t nama = LIST_HEAD_INIT(nama)

/* Inisialisasi list head dinamis */
#define list_head_init(head) \
    do { \
        (head)->berikutnya = (head); \
        (head)->sebelumnya = (head); \
    } while (0)

/* Tambah entry baru di antara dua entry */
#define __list_tambah(baru, sebelumnya, berikutnya) \
    do { \
        (berikutnya)->sebelumnya = (baru); \
        (baru)->berikutnya = (berikutnya); \
        (baru)->sebelumnya = (sebelumnya); \
        (sebelumnya)->berikutnya = (baru); \
    } while (0)

/* Tambah di depan list */
#define list_tambah_depan(baru, head) \
    __list_tambah(baru, head, (head)->berikutnya)

/* Tambah di belakang list */
#define list_tambah_belakang(baru, head) \
    __list_tambah(baru, (head)->sebelumnya, head)

/* Hapus entry dari list */
#define list_hapus(entry) \
    do { \
        (entry)->berikutnya->sebelumnya = (entry)->sebelumnya; \
        (entry)->sebelumnya->berikutnya = (entry)->berikutnya; \
        (entry)->berikutnya = (entry); \
        (entry)->sebelumnya = (entry); \
    } while (0)

/* Cek apakah list kosong */
#define list_kosong(head) ((head)->berikutnya == (head))

/* Dapatkan entry dari list member */
#define list_entry(ptr, tipe, member) \
    CONTAINER_OF(ptr, tipe, member)

/* Iterasi list */
#define list_for_each(pos, head) \
    for ((pos) = (head)->berikutnya; \
         (pos) != (head); \
         (pos) = (pos)->berikutnya)

/* Iterasi list dengan aman */
#define list_for_each_aman(pos, n, head) \
    for ((pos) = (head)->berikutnya, (n) = (pos)->berikutnya; \
         (pos) != (head); \
         (pos) = (n), (n) = (pos)->berikutnya)

/* Dapatkan first entry */
#define list_first_entry(head, tipe, member) \
    list_entry((head)->berikutnya, tipe, member)

/* Dapatkan last entry */
#define list_last_entry(head, tipe, member) \
    list_entry((head)->sebelumnya, tipe, member)

/*
 * ===========================================================================
 * MAKRO SPINLOCK (SPINLOCK MACROS)
 * ===========================================================================
 */

/* Struktur spinlock */
typedef struct {
    volatile tak_bertanda32_t terkunci;
} spinlock_t;

/* Inisialisasi spinlock statis */
#define SPINLOCK_INIT { 0 }

/* Inisialisasi spinlock dinamis */
#define spinlock_init(lock) \
    do { \
        (lock)->terkunci = 0; \
    } while (0)

/* Lock spinlock (busy wait) */
#define spinlock_kunci(lock) \
    do { \
        while (__sync_lock_test_and_set(&(lock)->terkunci, 1)) { \
            while ((lock)->terkunci) { \
                __asm__ __volatile__("pause"); \
            } \
        } \
    } while (0)

/* Unlock spinlock */
#define spinlock_buka(lock) \
    do { \
        __sync_lock_release(&(lock)->terkunci); \
    } while (0)

/* Cek apakah spinlock terkunci */
#define spinlock_terkunci(lock) ((lock)->terkunci != 0)

/* Try lock */
#define spinlock_coba(lock) \
    (__sync_lock_test_and_set(&(lock)->terkunci, 1) == 0)

/*
 * ===========================================================================
 * MAKRO RING BUFFER (RING BUFFER MACROS)
 * ===========================================================================
 */

/* Struktur ring buffer */
typedef struct {
    tak_bertanda8_t *data;
    ukuran_t ukuran;
    volatile ukuran_t kepala;
    volatile ukuran_t ekor;
} ring_buffer_t;

/* Inisialisasi ring buffer */
#define ring_buffer_init(rb, buffer, ukuran_buf) \
    do { \
        (rb)->data = (buffer); \
        (rb)->ukuran = (ukuran_buf); \
        (rb)->kepala = 0; \
        (rb)->ekor = 0; \
    } while (0)

/* Cek apakah ring buffer kosong */
#define ring_buffer_kosong(rb) ((rb)->kepala == (rb)->ekor)

/* Cek apakah ring buffer penuh */
#define ring_buffer_penuh(rb) \
    (((rb)->kepala + 1) % (rb)->ukuran == (rb)->ekor)

/* Dapatkan jumlah data dalam buffer */
#define ring_buffer_terisi(rb) \
    (((rb)->kepala - (rb)->ekor + (rb)->ukuran) % (rb)->ukuran)

/* Dapatkan ruang tersedia */
#define ring_buffer_tersedia(rb) \
    ((rb)->ukuran - 1 - ring_buffer_terisi(rb))

/*
 * ===========================================================================
 * MAKRO HASH (HASH MACROS)
 * ===========================================================================
 */

/* Hash string init value */
#define HASH_STRING_INIT 5381

/* Hash step untuk string (djb2) */
#define HASH_STRING_STEP(hash, c) \
    (((hash) << 5) + (hash) + (c))

/*
 * ===========================================================================
 * MAKRO BOUNDARY CHECK
 * ===========================================================================
 */

/* Cek apakah nilai dalam rentang [min, maks] (inklusif) */
#define DALAM_RENTANG(val, min, maks) \
    (((val) >= (min)) && ((val) <= (maks)))

/* Cek apakah nilai dalam rentang [min, maks) (setengah terbuka) */
#define DALAM_RENTANG_TERBUKA(val, min, maks) \
    (((val) >= (min)) && ((val) < (maks)))

/* Cek apakah index valid dalam array */
#define INDEX_VALID(idx, arr) \
    DALAM_RENTANG_TERBUKA(idx, 0, ARRAY_UKURAN(arr))

/* Cek apakah pointer dalam range */
#define PTR_IN_RANGE(ptr, mulai, akhir) \
    (((uintptr_t)(ptr) >= (uintptr_t)(mulai)) && \
     ((uintptr_t)(ptr) < (uintptr_t)(akhir)))

/*
 * ===========================================================================
 * MAKRO SECTION DAN ATTRIBUTE
 * ===========================================================================
 */

/* Section attributes */
#define SECTION(s) __attribute__((section(s)))

/* Function attributes */
#define NOINLINE __attribute__((noinline))
#define MURNI __attribute__((pure))
#define KONSTAN __attribute__((const))
#define PACKED __attribute__((packed))
#define ALIGNED(n) __attribute__((aligned(n)))
#define DIGUNAKAN __attribute__((used))
#define USANG __attribute__((deprecated))
#define LEMAH __attribute__((weak))

/* Constructor/Destructor */
#define KONSTRUKTOR __attribute__((constructor))
#define DESTRUKTOR __attribute__((destructor))

#endif /* INTI_MAKRO_H */
