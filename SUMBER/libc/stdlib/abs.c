/*
 * PIGURA LIBC - STDLIB/ABS.C
 * ===========================
 * Implementasi fungsi aritmetika integer.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - abs()   : Nilai absolut int
 *   - labs()  : Nilai absolut long
 *   - llabs() : Nilai absolut long long
 *   - div()   : Pembagian int dengan quotient dan remainder
 *   - ldiv()  : Pembagian long
 *   - lldiv() : Pembagian long long
 */

#include <stdlib.h>
#include <limits.h>

/* ============================================================
 * IMPLEMENTASI FUNGSI ABSOLUT
 * ============================================================
 */

/*
 * abs - Nilai absolut int
 *
 * Parameter:
 *   n - Nilai input
 *
 * Return: Nilai absolut
 *
 * Catatan: Hasil tidak terdefinisi untuk INT_MIN karena
 *          -INT_MIN tidak dapat direpresentasikan.
 */
int abs(int n) {
    /* Perhatikan: abs(INT_MIN) adalah undefined behavior */
    return (n < 0) ? -n : n;
}

/*
 * labs - Nilai absolut long
 *
 * Parameter:
 *   n - Nilai input
 *
 * Return: Nilai absolut
 */
long labs(long n) {
    /* Perhatikan: labs(LONG_MIN) adalah undefined behavior */
    return (n < 0) ? -n : n;
}

/*
 * llabs - Nilai absolut long long (C99)
 *
 * Parameter:
 *   n - Nilai input
 *
 * Return: Nilai absolut
 */
long long llabs(long long n) {
    /* Perhatikan: llabs(LLONG_MIN) adalah undefined behavior */
    return (n < 0) ? -n : n;
}

/* ============================================================
 * IMPLEMENTASI FUNGSI DIVISION
 * ============================================================
 */

/*
 * div - Pembagian int dengan hasil lengkap
 *
 * Parameter:
 *   numer - Pembilang
 *   denom - Penyebut
 *
 * Return: Struktur div_t dengan quot dan rem
 *
 * Catatan: Pembagian dengan 0 menghasilkan undefined behavior.
 *          Untuk INT_MIN / -1, hasil undefined pada arsitektur
 *          32-bit karena overflow.
 */
div_t div(int numer, int denom) {
    div_t result;

    /* Penanganan khusus untuk INT_MIN / -1 dihindari karena
     * undefined behavior menurut standar C */
    result.quot = numer / denom;
    result.rem = numer % denom;

    /* Standar C menjamin: quot*denom + rem == numer
     * dan |rem| < |denom|.
     * Di C99+, rem memiliki tanda yang sama dengan numer.
     * Di C89, implementasi-defined. */

    return result;
}

/*
 * ldiv - Pembagian long dengan hasil lengkap
 *
 * Parameter:
 *   numer - Pembilang
 *   denom - Penyebut
 *
 * Return: Struktur ldiv_t dengan quot dan rem
 */
ldiv_t ldiv(long numer, long denom) {
    ldiv_t result;

    result.quot = numer / denom;
    result.rem = numer % denom;

    return result;
}

/*
 * lldiv - Pembagian long long dengan hasil lengkap (C99)
 *
 * Parameter:
 *   numer - Pembilang
 *   denom - Penyebut
 *
 * Return: Struktur lldiv_t dengan quot dan rem
 */
lldiv_t lldiv(long long numer, long long denom) {
    lldiv_t result;

    result.quot = numer / denom;
    result.rem = numer % denom;

    return result;
}

/* ============================================================
 * FUNGSI TAMBAHAN (NON-STANDAR)
 * ============================================================
 */

/*
 * imaxabs - Nilai absolut intmax_t (C99)
 *
 * Parameter:
 *   n - Nilai input
 *
 * Return: Nilai absolut
 *
 * Catatan: Ini adalah alias untuk llabs karena intmax_t
 *          adalah long long di platform ini.
 */
long long imaxabs(long long n) {
    return llabs(n);
}

/*
 * imaxdiv - Pembagian intmax_t (C99)
 *
 * Parameter:
 *   numer - Pembilang
 *   denom - Penyebut
 *
 * Return: Struktur imaxdiv_t dengan quot dan rem
 */
typedef struct {
    long long quot;
    long long rem;
} imaxdiv_t;

imaxdiv_t imaxdiv(long long numer, long long denom) {
    imaxdiv_t result;

    result.quot = numer / denom;
    result.rem = numer % denom;

    return result;
}
