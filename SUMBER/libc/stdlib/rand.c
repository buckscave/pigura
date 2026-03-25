/*
 * PIGURA LIBC - STDLIB/RAND.C
 * ============================
 * Implementasi fungsi random number generator.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - rand()  : Generate random number
 *   - srand() : Set seed untuk RNG
 *
 * Implementasi menggunakan Linear Congruential Generator (LCG)
 * dengan parameter dari POSIX.1-2001 (aka "TYPE0" generator).
 *
 * Formula: next = (prev * 1103515245 + 12345) mod 2^31
 *
 * Catatan: LCG bukan cryptographically secure. Untuk keperluan
 * kriptografi, gunakan sumber random yang lebih aman.
 */

#include <stdlib.h>

/* ============================================================
 * KONFIGURASI LCG
 * ============================================================
 */

/* Parameter LCG (POSIX.1-2001) */
#define LCG_MULTIPLIER  1103515245UL
#define LCG_INCREMENT   12345UL
#define LCG_MODULUS     2147483648UL  /* 2^31 */

/* ============================================================
 * VARIABEL GLOBAL
 * ============================================================
 */

/* State RNG - diinisialisasi dengan seed default 1 */
static unsigned long rand_state = 1;

/* Flag untuk menandai apakah srand sudah dipanggil */
static int rand_seeded = 0;

/* ============================================================
 * IMPLEMENTASI FUNGSI
 * ============================================================
 */

/*
 * srand - Set seed untuk random number generator
 *
 * Parameter:
 *   seed - Nilai seed
 *
 * Jika seed = 0, gunakan seed default (1).
 * Menginisialisasi state RNG dengan nilai seed.
 */
void srand(unsigned int seed) {
    /* Seed 0 diperlakukan sebagai seed 1 */
    if (seed == 0) {
        rand_state = 1;
    } else {
        rand_state = seed;
    }
    rand_seeded = 1;
}

/*
 * rand - Generate random number
 *
 * Return: Integer random dalam rentang [0, RAND_MAX]
 *
 * Menggunakan Linear Congruential Generator untuk menghasilkan
 * bilangan pseudo-random. Rentang nilai adalah 0 hingga
 * RAND_MAX (32767) sesuai standar C.
 */
int rand(void) {
    /* Inisialisasi dengan seed default jika belum */
    if (!rand_seeded) {
        rand_state = 1;
        rand_seeded = 1;
    }

    /* Update state menggunakan LCG */
    rand_state = (rand_state * LCG_MULTIPLIER + LCG_INCREMENT)
                 % LCG_MODULUS;

    /* Return nilai dalam rentang [0, RAND_MAX]
     * RAND_MAX = 32767 = 2^15 - 1
     * rand_state dalam rentang [0, 2^31 - 1]
     * Kita ambil 15 bit teratas untuk distribusi yang lebih baik */
    return (int)((rand_state >> 16) & RAND_MAX);
}

/* ============================================================
 * FUNGSI TAMBAHAN (NON-STANDAR)
 * ============================================================
 */

/*
 * rand_r - Thread-safe random number generator (POSIX)
 *
 * Parameter:
 *   seedp - Pointer ke state/seed (diupdate oleh fungsi)
 *
 * Return: Integer random dalam rentang [0, RAND_MAX]
 *
 * Catatan: Fungsi ini tidak mengubah global state,
 *          sehingga aman untuk multi-threading.
 */
int rand_r(unsigned int *seedp) {
    unsigned long state;

    /* Validasi parameter */
    if (seedp == NULL) {
        return 0;
    }

    state = *seedp;

    /* Update state menggunakan LCG yang sama */
    state = (state * LCG_MULTIPLIER + LCG_INCREMENT) % LCG_MODULUS;

    *seedp = (unsigned int)state;

    return (int)((state >> 16) & RAND_MAX);
}

/*
 * random - Random number generator yang lebih baik (BSD)
 *
 * Return: Integer random dalam rentang [0, 2^31 - 1]
 *
 * Catatan: Menggunakan algoritma yang sama dengan rand()
 *          tapi mengembalikan nilai yang lebih besar.
 */
long random(void) {
    /* Inisialisasi dengan seed default jika belum */
    if (!rand_seeded) {
        rand_state = 1;
        rand_seeded = 1;
    }

    /* Update state */
    rand_state = (rand_state * LCG_MULTIPLIER + LCG_INCREMENT)
                 % LCG_MODULUS;

    return (long)rand_state;
}

/*
 * srandom - Set seed untuk random() (BSD)
 *
 * Parameter:
 *   seed - Nilai seed
 */
void srandom(unsigned int seed) {
    srand(seed);
}

/*
 * initstate - Inisialisasi state untuk random() (BSD)
 *
 * Parameter:
 *   seed      - Nilai seed awal
 *   statebuf  - Buffer untuk state
 *   statelen  - Panjang buffer
 *
 * Return: Pointer ke state sebelumnya
 */
char *initstate(unsigned int seed, char *statebuf, size_t statelen) {
    /* Implementasi sederhana - hanya gunakan state global */
    (void)statebuf;
    (void)statelen;

    rand_state = seed ? seed : 1;
    rand_seeded = 1;

    return (char *)&rand_state;
}

/*
 * setstate - Set state untuk random() (BSD)
 *
 * Parameter:
 *   statebuf - Buffer state baru
 *
 * Return: Pointer ke state sebelumnya
 */
char *setstate(char *statebuf) {
    unsigned long *new_state;

    if (statebuf == NULL) {
        return (char *)&rand_state;
    }

    new_state = (unsigned long *)statebuf;
    rand_state = *new_state;

    return (char *)&rand_state;
}
