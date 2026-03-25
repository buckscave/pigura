/*
 * PIGURA LIBC - STDLIB/QSORT.C
 * =============================
 * Implementasi fungsi quicksort.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - qsort() : Quick sort algorithm
 *   - qsort_r() : Quick sort dengan user context (BSD)
 *
 * Implementasi menggunakan algoritma quicksort dengan:
 *   - Median-of-three pivot selection
 *   - Insertion sort untuk partisi kecil
 *   - Stack-based iterative approach untuk menghindari
 *     stack overflow pada data besar
 */

#include <stdlib.h>
#include <string.h>

/* ============================================================
 * KONFIGURASI
 * ============================================================
 */

/* Ukuran minimum untuk insertion sort */
#define INSERTION_SORT_THRESHOLD 16

/* Maksimum depth untuk stack iterative */
#define MAX_STACK_SIZE 64

/* ============================================================
 * FUNGSI PEMBANTU
 * ============================================================
 */

/*
 * swap_bytes - Tukar dua blok memori
 *
 * Parameter:
 *   a    - Pointer ke blok pertama
 *   b    - Pointer ke blok kedua
 *   size - Ukuran blok dalam byte
 */
static void swap_bytes(void *a, void *b, size_t size) {
    unsigned char *pa = (unsigned char *)a;
    unsigned char *pb = (unsigned char *)b;
    unsigned char temp;
    size_t i;

    for (i = 0; i < size; i++) {
        temp = pa[i];
        pa[i] = pb[i];
        pb[i] = temp;
    }
}

/*
 * insertion_sort - Insertion sort untuk partisi kecil
 *
 * Parameter:
 *   base   - Pointer ke array
 *   nmemb  - Jumlah elemen
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan
 */
static void insertion_sort(void *base, size_t nmemb, size_t size,
                           int (*compar)(const void *, const void *)) {
    unsigned char *arr = (unsigned char *)base;
    size_t i, j;
    unsigned char *key;

    /* Buffer untuk menyimpan elemen yang diinsert */
    unsigned char *temp = (unsigned char *)malloc(size);
    if (temp == NULL) {
        /* Fallback ke bubble sort sederhana jika malloc gagal */
        for (i = 0; i < nmemb - 1; i++) {
            for (j = 0; j < nmemb - i - 1; j++) {
                unsigned char *a = arr + j * size;
                unsigned char *b = arr + (j + 1) * size;
                if (compar(a, b) > 0) {
                    swap_bytes(a, b, size);
                }
            }
        }
        return;
    }

    for (i = 1; i < nmemb; i++) {
        key = arr + i * size;
        memcpy(temp, key, size);

        j = i;
        while (j > 0) {
            unsigned char *prev = arr + (j - 1) * size;
            if (compar(prev, temp) > 0) {
                memcpy(arr + j * size, prev, size);
                j--;
            } else {
                break;
            }
        }

        memcpy(arr + j * size, temp, size);
    }

    free(temp);
}

/*
 * insertion_sort_r - Insertion sort dengan context
 *
 * Parameter:
 *   base   - Pointer ke array
 *   nmemb  - Jumlah elemen
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan dengan context
 *   arg    - User context
 */
static void insertion_sort_r(void *base, size_t nmemb, size_t size,
                             int (*compar)(const void *, const void *,
                                          void *),
                             void *arg) {
    unsigned char *arr = (unsigned char *)base;
    size_t i, j;
    unsigned char *key;

    unsigned char *temp = (unsigned char *)malloc(size);
    if (temp == NULL) {
        for (i = 0; i < nmemb - 1; i++) {
            for (j = 0; j < nmemb - i - 1; j++) {
                unsigned char *a = arr + j * size;
                unsigned char *b = arr + (j + 1) * size;
                if (compar(a, b, arg) > 0) {
                    swap_bytes(a, b, size);
                }
            }
        }
        return;
    }

    for (i = 1; i < nmemb; i++) {
        key = arr + i * size;
        memcpy(temp, key, size);

        j = i;
        while (j > 0) {
            unsigned char *prev = arr + (j - 1) * size;
            if (compar(prev, temp, arg) > 0) {
                memcpy(arr + j * size, prev, size);
                j--;
            } else {
                break;
            }
        }

        memcpy(arr + j * size, temp, size);
    }

    free(temp);
}

/*
 * median_of_three - Pilih pivot dengan median-of-three
 *
 * Parameter:
 *   base   - Pointer ke array
 *   lo     - Indeks rendah
 *   hi     - Indeks tinggi
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan
 *
 * Return: Indeks pivot terpilih
 */
static size_t median_of_three(void *base, size_t lo, size_t hi,
                              size_t size,
                              int (*compar)(const void *, const void *)) {
    unsigned char *arr = (unsigned char *)base;
    size_t mid = lo + (hi - lo) / 2;
    unsigned char *a, *b, *c;

    a = arr + lo * size;
    b = arr + mid * size;
    c = arr + hi * size;

    /* Cari median dari tiga elemen */
    if (compar(a, b) > 0) {
        if (compar(b, c) > 0) {
            return mid;      /* a > b > c, median = b */
        } else if (compar(a, c) > 0) {
            return hi;       /* a > c > b, median = c */
        } else {
            return lo;       /* c > a > b, median = a */
        }
    } else {
        if (compar(a, c) > 0) {
            return lo;       /* b > a > c, median = a */
        } else if (compar(b, c) > 0) {
            return hi;       /* b > c > a, median = c */
        } else {
            return mid;      /* c > b > a, median = b */
        }
    }
}

/*
 * partition - Partisi array untuk quicksort
 *
 * Parameter:
 *   base   - Pointer ke array
 *   lo     - Indeks rendah
 *   hi     - Indeks tinggi
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan
 *
 * Return: Indeks pivot setelah partisi
 */
static size_t partition(void *base, size_t lo, size_t hi, size_t size,
                        int (*compar)(const void *, const void *)) {
    unsigned char *arr = (unsigned char *)base;
    size_t pivot_idx;
    unsigned char *pivot;
    size_t i, j;

    /* Pilih pivot dengan median-of-three */
    pivot_idx = median_of_three(base, lo, hi, size, compar);

    /* Pindahkan pivot ke akhir */
    swap_bytes(arr + pivot_idx * size, arr + hi * size, size);
    pivot = arr + hi * size;

    /* Partisi */
    i = lo;
    for (j = lo; j < hi; j++) {
        if (compar(arr + j * size, pivot) <= 0) {
            if (i != j) {
                swap_bytes(arr + i * size, arr + j * size, size);
            }
            i++;
        }
    }

    /* Pindahkan pivot ke posisi final */
    swap_bytes(arr + i * size, arr + hi * size, size);

    return i;
}

/*
 * partition_r - Partisi dengan context
 */
static size_t partition_r(void *base, size_t lo, size_t hi, size_t size,
                          int (*compar)(const void *, const void *, void *),
                          void *arg) {
    unsigned char *arr = (unsigned char *)base;
    size_t i, j;
    unsigned char *pivot;

    /* Pilih pivot sederhana untuk versi r */
    pivot = arr + hi * size;

    i = lo;
    for (j = lo; j < hi; j++) {
        if (compar(arr + j * size, pivot, arg) <= 0) {
            if (i != j) {
                swap_bytes(arr + i * size, arr + j * size, size);
            }
            i++;
        }
    }

    swap_bytes(arr + i * size, arr + hi * size, size);

    return i;
}

/* ============================================================
 * IMPLEMENTASI FUNGSI UTAMA
 * ============================================================
 */

/*
 * qsort - Quick sort
 *
 * Parameter:
 *   base   - Array yang diurutkan
 *   nmemb  - Jumlah elemen
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan
 *
 * Mengurutkan array dalam urutan menaik.
 * Fungsi compar mengembalikan:
 *   < 0 jika a < b
 *   = 0 jika a == b
 *   > 0 jika a > b
 */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *)) {
    /* Stack untuk iterative implementation */
    size_t lo_stack[MAX_STACK_SIZE];
    size_t hi_stack[MAX_STACK_SIZE];
    int stack_top = 0;

    size_t lo, hi, p;

    /* Validasi parameter */
    if (base == NULL || nmemb < 2 || size == 0 || compar == NULL) {
        return;
    }

    /* Inisialisasi stack */
    lo_stack[stack_top] = 0;
    hi_stack[stack_top] = nmemb - 1;
    stack_top++;

    /* Iterative quicksort */
    while (stack_top > 0) {
        /* Pop dari stack */
        stack_top--;
        lo = lo_stack[stack_top];
        hi = hi_stack[stack_top];

        /* Gunakan insertion sort untuk partisi kecil */
        if (hi - lo + 1 <= INSERTION_SORT_THRESHOLD) {
            insertion_sort((unsigned char *)base + lo * size,
                          hi - lo + 1, size, compar);
            continue;
        }

        /* Partisi */
        p = partition(base, lo, hi, size, compar);

        /* Push partisi yang lebih kecil dulu untuk efisiensi stack */
        if (p > lo + 1) {
            lo_stack[stack_top] = lo;
            hi_stack[stack_top] = p - 1;
            stack_top++;
        }

        if (p + 1 < hi) {
            lo_stack[stack_top] = p + 1;
            hi_stack[stack_top] = hi;
            stack_top++;
        }
    }
}

/*
 * qsort_r - Quick sort dengan user context (BSD/GNU)
 *
 * Parameter:
 *   base   - Array yang diurutkan
 *   nmemb  - Jumlah elemen
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan dengan context
 *   arg    - User context yang dilewatkan ke compar
 */
void qsort_r(void *base, size_t nmemb, size_t size,
             int (*compar)(const void *, const void *, void *),
             void *arg) {
    size_t lo_stack[MAX_STACK_SIZE];
    size_t hi_stack[MAX_STACK_SIZE];
    int stack_top = 0;

    size_t lo, hi, p;

    /* Validasi parameter */
    if (base == NULL || nmemb < 2 || size == 0 || compar == NULL) {
        return;
    }

    /* Inisialisasi stack */
    lo_stack[stack_top] = 0;
    hi_stack[stack_top] = nmemb - 1;
    stack_top++;

    /* Iterative quicksort */
    while (stack_top > 0) {
        stack_top--;
        lo = lo_stack[stack_top];
        hi = hi_stack[stack_top];

        if (hi - lo + 1 <= INSERTION_SORT_THRESHOLD) {
            insertion_sort_r((unsigned char *)base + lo * size,
                            hi - lo + 1, size, compar, arg);
            continue;
        }

        p = partition_r(base, lo, hi, size, compar, arg);

        if (p > lo + 1) {
            lo_stack[stack_top] = lo;
            hi_stack[stack_top] = p - 1;
            stack_top++;
        }

        if (p + 1 < hi) {
            lo_stack[stack_top] = p + 1;
            hi_stack[stack_top] = hi;
            stack_top++;
        }
    }
}
