/*
 * PIGURA OS - MAKRO.H
 * -------------------
 * Definisi makro utilitas untuk kernel Pigura OS.
 *
 * Berkas ini berisi makro-makro umum yang digunakan di seluruh kernel
 * untuk operasi bitwise, pengecekan, manipulasi string, dan utilitas lain.
 *
 * Versi: 1.0
 * Tanggal: 2025
 *
 * CATATAN: Semua makro di berkas ini adalah C90 compliant.
 *          Jangan menggunakan typeof atau fitur non-standard.
 */

#ifndef INTI_MAKRO_H
#define INTI_MAKRO_H

/* Include tipe dasar */
#include "types.h"

/*
 * ============================================================================
 * MAKRO UTILITAS UMUM (GENERAL UTILITY MACROS)
 * ============================================================================
 */

/* Mendapatkan ukuran array dalam elemen */
#define ARRAY_UKURAN(arr)       (sizeof(arr) / sizeof((arr)[0]))

/* Mendapatkan offset member dalam struct */
#define OFFSET_OF(tipe, member) ((ukuran_t) &((tipe *)0)->member)

/* Mendapatkan struct dari pointer member (C90 compliant) */
#define CONTAINER_OF(ptr, tipe, member) \
    ((tipe *)((char *)(ptr) - OFFSET_OF(tipe, member)))

/* Nilai minimum dua bilangan */
#define MIN(a, b)               (((a) < (b)) ? (a) : (b))

/* Nilai maksimum dua bilangan */
#define MAKS(a, b)              (((a) > (b)) ? (a) : (b))

/* Batasi nilai dalam rentang */
#define BATAS(val, min, maks)   (MIN(MAKS(val, min), maks))

/* Tukar dua nilai (tipisahkan tipe eksplisit) */
#define TUKAR(a, b, tipe)       \
    do {                        \
        tipe _tmp = (a);        \
        (a) = (b);              \
        (b) = _tmp;             \
    } while (0)

/* Cek apakah nilai adalah pangkat dua */
#define ADA_PANGKAT_DUA(n)      (((n) > 0) && (((n) & ((n) - 1)) == 0))

/* Bulatkan ke pangkat dua berikutnya */
#define BULATKAN_PANGKAT_DUA(n) \
    (((n) > 0) ? (1UL << (32 - __builtin_clz((n) - 1))) : 1UL)

/*
 * ============================================================================
 * MAKRO BITWISE (BITWISE MACROS)
 * ============================================================================
 */

/* Set bit */
#define BIT_SET(reg, bit)       ((reg) |= (1UL << (bit)))

/* Clear bit */
#define BIT_CLEAR(reg, bit)     ((reg) &= ~(1UL << (bit)))

/* Toggle bit */
#define BIT_TOGGLE(reg, bit)    ((reg) ^= (1UL << (bit)))

/* Cek apakah bit set */
#define BIT_TEST(reg, bit)      (((reg) >> (bit)) & 1UL)

/* Set beberapa bit */
#define BITS_SET(reg, mask)     ((reg) |= (mask))

/* Clear beberapa bit */
#define BITS_CLEAR(reg, mask)   ((reg) &= ~(mask))

/* Dapatkan nilai bit dalam rentang */
#define BITS_GET(reg, mulai, lebar) \
    (((reg) >> (mulai)) & ((1UL << (lebar)) - 1UL))

/* Set nilai bit dalam rentang */
#define BITS_SET_NILAI(reg, mulai, lebar, nilai) \
    do {                                         \
        tak_bertanda32_t _mask = ((1UL << (lebar)) - 1UL); \
        (reg) = ((reg) & ~(_mask << (mulai))) | \
                (((nilai) & _mask) << (mulai)); \
    } while (0)

/* Mask untuk bit rendah */
#define MASK_BAWAH(n)           ((1UL << (n)) - 1UL)

/* Mask untuk bit tinggi */
#define MASK_ATAS(n)            (~((1UL << (n)) - 1UL))

/* Byte low dan high */
#define BYTE_RENDAH(w)          ((tak_bertanda8_t)((w) & 0xFF))
#define BYTE_TINGGI(w)          ((tak_bertanda8_t)(((w) >> 8) & 0xFF))

/* Word low dan high */
#define KATA_RENDAH(d)          ((tak_bertanda16_t)((d) & 0xFFFF))
#define KATA_TINGGI(d)          ((tak_bertanda16_t)(((d) >> 16) & 0xFFFF))

/* Dword low dan high */
#define DWITA_RENDAH(q)         ((tak_bertanda32_t)((q) & 0xFFFFFFFFUL))
#define DWITA_TINGGI(q)         ((tak_bertanda32_t)(((q) >> 32) & 0xFFFFFFFFUL))

/* Gabungkan byte menjadi word */
#define GABUNG_KATA(lo, hi)     ((tak_bertanda16_t)(((hi) << 8) | (lo)))

/* Gabungkan word menjadi dword */
#define GABUNG_DWITA(lo, hi)    ((tak_bertanda32_t)(((hi) << 16) | (lo)))

/* Gabungkan dword menjadi qword */
#define GABUNG_KWITA(lo, hi)    ((tak_bertanda64_t)( \
    ((tak_bertanda64_t)(hi) << 32) | (lo)))

/*
 * ============================================================================
 * MAKRO ALIGNMENT (ALIGNMENT MACROS)
 * ============================================================================
 */

/* Ratakan ke atas ke batas alignment */
#define RATAKAN_ATAS(val, align) \
    (((val) + (align) - 1) & ~((align) - 1))

/* Ratakan ke bawah ke batas alignment */
#define RATAKAN_BAWAH(val, align) \
    ((val) & ~((align) - 1))

/* Cek apakah sudah aligned */
#define SUDAH_RATA(val, align)  (((val) & ((align) - 1)) == 0)

/* Dapatkan offset dari alignment */
#define OFFSET_RATA(val, align) ((val) & ((align) - 1))

/* Dapatkan alignment berikutnya */
#define RATAKAN_BERIKUTNYA(val, align) \
    (SUDAH_RATA(val, align) ? (val) : RATAKAN_ATAS(val, align))

/*
 * ============================================================================
 * MAKRO KONTROL ALUR (FLOW CONTROL MACROS)
 * ============================================================================
 */

/* Loop forever */
#define SELAMA_LAMA()           for (;;)

/* Unreachable code */
#define TIDAK_TERJANGKAU()      for (;;)

/* Statement yang tidak digunakan */
#define TIDAK_DIGUNAKAN(x)      ((void)(x))

/* Barrier memori */
#define BARRIER_MEMORI()        __asm__ __volatile__("" ::: "memory")

/* Barrier baca */
#define BARRIER_BACA()          __asm__ __volatile__("" ::: "memory")

/* Barrier tulis */
#define BARRIER_TULIS()         __asm__ __volatile__("" ::: "memory")

/* Compiler hint untuk kondisi yang sering benar */
#define KEMUNGKINAN(x)          __builtin_expect(!!(x), 1)

/* Compiler hint untuk kondisi yang jarang benar */
#define JARANG(x)               __builtin_expect(!!(x), 0)

/*
 * ============================================================================
 * MAKRO STRING (STRING MACROS)
 * ============================================================================
 */

/* Konversi ke string */
#define STRINGIFY(x)            #x
#define TOSTRING(x)             STRINGIFY(x)

/* Konversi karakter ke digit */
#define CHAR_KE_DIGIT(c)        ((c) - '0')

/* Konversi digit ke karakter */
#define DIGIT_KE_CHAR(d)        ((d) + '0')

/* Konversi huruf ke uppercase */
#define KE_UPPER(c)             (((c) >= 'a' && (c) <= 'z') ? \
                                 ((c) - 32) : (c))

/* Konversi huruf ke lowercase */
#define KE_LOWER(c)             (((c) >= 'A' && (c) <= 'Z') ? \
                                 ((c) + 32) : (c))

/* Cek apakah karakter adalah digit */
#define ADALAH_DIGIT(c)         ((c) >= '0' && (c) <= '9')

/* Cek apakah karakter adalah huruf */
#define ADALAH_HURUF(c)         (((c) >= 'a' && (c) <= 'z') || \
                                 ((c) >= 'A' && (c) <= 'Z'))

/* Cek apakah karakter adalah alphanumeric */
#define ADALAH_ALNUM(c)         (ADALAH_HURUF(c) || ADALAH_DIGIT(c))

/* Cek apakah karakter adalah whitespace */
#define ADALAH_WHITESPACE(c)    ((c) == ' ' || (c) == '\t' || \
                                 (c) == '\n' || (c) == '\r')

/* Cek apakah karakter adalah hex digit */
#define ADALAH_HEXDIGIT(c)      (ADALAH_DIGIT(c) || \
                                 ((c) >= 'a' && (c) <= 'f') || \
                                 ((c) >= 'A' && (c) <= 'F'))

/*
 * ============================================================================
 * MAKRO DEBUG (DEBUG MACROS)
 * ============================================================================
 */

/* Static assertion untuk compile-time check (C90 compliant) */
#define STATIC_ASSERT(cond, nama) \
    typedef char static_assert_##nama[(cond) ? 1 : -1]

/*
 * ============================================================================
 * MAKRO ERROR HANDLING (ERROR HANDLING MACROS)
 * ============================================================================
 */

/* Cek dan return jika error */
#define CEK_ERROR(expr)                 \
    do {                                \
        status_t _status = (expr);      \
        if (_status != STATUS_BERHASIL) \
            return _status;             \
    } while (0)

/* Cek dan goto jika error */
#define CEK_ERROR_GOTO(expr, label)     \
    do {                                \
        status_t _status = (expr);      \
        if (_status != STATUS_BERHASIL) \
            goto label;                 \
    } while (0)

/* Cek pointer dan return error */
#define CEK_PTR(ptr)                    \
    do {                                \
        if ((ptr) == NULL)              \
            return STATUS_PARAM_INVALID;\
    } while (0)

/* Cek pointer dan goto error */
#define CEK_PTR_GOTO(ptr, label)        \
    do {                                \
        if ((ptr) == NULL)              \
            goto label;                 \
    } while (0)

/* Cek alokasi memori */
#define CEK_ALOKASI(ptr)                \
    do {                                \
        if ((ptr) == NULL)              \
            return STATUS_MEMORI_HABIS; \
    } while (0)

/*
 * ============================================================================
 * STRUKTUR DAN FUNGSI LINKED LIST (LINKED LIST)
 * ============================================================================
 */

/* Struktur list head */
struct list_head {
    struct list_head *berikutnya;
    struct list_head *sebelumnya;
};

/* Inisialisasi list head */
#define LIST_HEAD_INIT(nama) { &(nama), &(nama) }

/* Deklarasi list head */
#define LIST_HEAD(nama) \
    struct list_head nama = LIST_HEAD_INIT(nama)

/* Inisialisasi list head dinamis */
static inline void list_head_init(struct list_head *head)
{
    head->berikutnya = head;
    head->sebelumnya = head;
}

/* Tambah entry di depan head */
static inline void list_tambah(struct list_head *baru,
                               struct list_head *sebelumnya,
                               struct list_head *berikutnya)
{
    berikutnya->sebelumnya = baru;
    baru->berikutnya = berikutnya;
    baru->sebelumnya = sebelumnya;
    sebelumnya->berikutnya = baru;
}

/* Tambah di depan list */
static inline void list_tambah_depan(struct list_head *baru,
                                     struct list_head *head)
{
    list_tambah(baru, head, head->berikutnya);
}

/* Tambah di belakang list */
static inline void list_tambah_belakang(struct list_head *baru,
                                        struct list_head *head)
{
    list_tambah(baru, head->sebelumnya, head);
}

/* Hapus entry dari list */
static inline void list_hapus(struct list_head *entry)
{
    entry->berikutnya->sebelumnya = entry->sebelumnya;
    entry->sebelumnya->berikutnya = entry->berikutnya;
    entry->berikutnya = entry;
    entry->sebelumnya = entry;
}

/* Cek apakah list kosong */
static inline int list_kosong(const struct list_head *head)
{
    return head->berikutnya == head;
}

/* Iterasi list */
#define list_for_each(pos, head) \
    for ((pos) = (head)->berikutnya; (pos) != (head); \
         (pos) = (pos)->berikutnya)

/* Iterasi list dengan aman (bisa hapus saat iterasi) */
#define list_for_each_aman(pos, n, head) \
    for ((pos) = (head)->berikutnya, (n) = (pos)->berikutnya; \
         (pos) != (head); \
         (pos) = (n), (n) = (pos)->berikutnya)

/* Dapatkan entry dari list */
#define list_entry(ptr, tipe, member) \
    CONTAINER_OF(ptr, tipe, member)

/*
 * ============================================================================
 * STRUKTUR DAN FUNGSI SPINLOCK (SPINLOCK)
 * ============================================================================
 */

/* Struktur spinlock */
typedef struct {
    volatile tak_bertanda32_t terkunci;
} spinlock_t;

/* Inisialisasi spinlock */
#define SPINLOCK_INIT { 0 }

/* Inisialisasi spinlock dinamis */
static inline void spinlock_init(spinlock_t *lock)
{
    lock->terkunci = 0;
}

/* Lock spinlock */
static inline void spinlock_kunci(spinlock_t *lock)
{
    while (__sync_lock_test_and_set(&lock->terkunci, 1)) {
        while (lock->terkunci) {
            __asm__ __volatile__("pause");
        }
    }
}

/* Unlock spinlock */
static inline void spinlock_buka(spinlock_t *lock)
{
    __sync_lock_release(&lock->terkunci);
}

/* Cek apakah spinlock terkunci */
static inline int spinlock_terkunci(spinlock_t *lock)
{
    return lock->terkunci != 0;
}

/*
 * ============================================================================
 * STRUKTUR DAN FUNGSI RING BUFFER (RING BUFFER)
 * ============================================================================
 */

/* Struktur ring buffer */
typedef struct {
    tak_bertanda8_t *data;
    ukuran_t ukuran;
    ukuran_t kepala;
    ukuran_t ekor;
} ring_buffer_t;

/* Inisialisasi ring buffer */
static inline void ring_buffer_init(ring_buffer_t *rb,
                                    tak_bertanda8_t *buffer,
                                    ukuran_t ukuran)
{
    rb->data = buffer;
    rb->ukuran = ukuran;
    rb->kepala = 0;
    rb->ekor = 0;
}

/* Tulis ke ring buffer, return jumlah byte yang ditulis */
static inline ukuran_t ring_buffer_tulis(ring_buffer_t *rb,
                                         const tak_bertanda8_t *data,
                                         ukuran_t len)
{
    ukuran_t i;
    ukuran_t tersedia;
    ukuran_t idx;

    tersedia = rb->ukuran - ((rb->kepala - rb->ekor) % rb->ukuran);
    if (tersedia == 0) {
        return 0;
    }

    len = MIN(len, tersedia);

    for (i = 0; i < len; i++) {
        idx = (rb->kepala + i) % rb->ukuran;
        rb->data[idx] = data[i];
    }

    rb->kepala = (rb->kepala + len) % rb->ukuran;

    return len;
}

/* Baca dari ring buffer, return jumlah byte yang dibaca */
static inline ukuran_t ring_buffer_baca(ring_buffer_t *rb,
                                        tak_bertanda8_t *data,
                                        ukuran_t len)
{
    ukuran_t i;
    ukuran_t tersedia;
    ukuran_t idx;

    tersedia = (rb->kepala - rb->ekor) % rb->ukuran;
    if (tersedia == 0) {
        return 0;
    }

    len = MIN(len, tersedia);

    for (i = 0; i < len; i++) {
        idx = (rb->ekor + i) % rb->ukuran;
        data[i] = rb->data[idx];
    }

    rb->ekor = (rb->ekor + len) % rb->ukuran;

    return len;
}

/* Cek apakah ring buffer kosong */
static inline int ring_buffer_kosong(const ring_buffer_t *rb)
{
    return rb->kepala == rb->ekor;
}

/* Dapatkan jumlah data dalam ring buffer */
static inline ukuran_t ring_buffer_terisi(const ring_buffer_t *rb)
{
    return (rb->kepala - rb->ekor) % rb->ukuran;
}

/*
 * ============================================================================
 * FUNGSI HASH (HASH FUNCTIONS)
 * ============================================================================
 */

/* Fungsi hash string sederhana (djb2) */
static inline tak_bertanda32_t hash_string(const char *str)
{
    tak_bertanda32_t hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

/* Fungsi hash integer */
static inline tak_bertanda32_t hash_int(tak_bertanda32_t key)
{
    key = ((key >> 16) ^ key) * 0x45D9F3B;
    key = ((key >> 16) ^ key) * 0x45D9F3B;
    key = (key >> 16) ^ key;
    return key;
}

/* Hash untuk pointer */
static inline tak_bertanda32_t hash_ptr(const void *ptr)
{
    return hash_int((tak_bertanda32_t)(ukuran_t)ptr);
}

#endif /* INTI_MAKRO_H */
