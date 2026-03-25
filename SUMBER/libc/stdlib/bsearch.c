/*
 * PIGURA LIBC - STDLIB/BSEARCH.C
 * ===============================
 * Implementasi fungsi binary search.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 *
 * Fungsi yang diimplementasikan:
 *   - bsearch()   : Binary search standar
 *   - bsearch_r() : Binary search dengan user context (BSD)
 *
 * Algoritma binary search mencari elemen dalam array terurut
 * dengan kompleksitas O(log n).
 */

#include <stdlib.h>

/* ============================================================
 * IMPLEMENTASI FUNGSI UTAMA
 * ============================================================
 */

/*
 * bsearch - Binary search
 *
 * Parameter:
 *   key    - Elemen yang dicari
 *   base   - Array awal (harus terurut)
 *   nmemb  - Jumlah elemen
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan
 *
 * Return: Pointer ke elemen yang ditemukan, atau NULL
 *
 * Fungsi compar mengembalikan:
 *   < 0 jika key < elemen
 *   = 0 jika key == elemen
 *   > 0 jika key > elemen
 *
 * Catatan: Array harus sudah terurut dalam urutan menaik.
 *          Jika ada elemen yang sama, elemen yang dikembalikan
 *          tidak ditentukan (bisa salah satu dari duplikat).
 */
void *bsearch(const void *key, const void *base, size_t nmemb,
              size_t size, int (*compar)(const void *, const void *)) {
    const unsigned char *arr = (const unsigned char *)base;
    size_t lo, hi, mid;
    int cmp;

    /* Validasi parameter */
    if (key == NULL || base == NULL || nmemb == 0 || size == 0 ||
        compar == NULL) {
        return NULL;
    }

    lo = 0;
    hi = nmemb - 1;

    /* Binary search iteratif */
    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;  /* Hindari overflow */

        cmp = compar(key, arr + mid * size);

        if (cmp == 0) {
            /* Elemen ditemukan */
            return (void *)(arr + mid * size);
        } else if (cmp < 0) {
            /* Key lebih kecil, cari di bagian kiri */
            if (mid == 0) {
                /* Hindari underflow */
                break;
            }
            hi = mid - 1;
        } else {
            /* Key lebih besar, cari di bagian kanan */
            lo = mid + 1;
        }
    }

    /* Elemen tidak ditemukan */
    return NULL;
}

/*
 * bsearch_r - Binary search dengan user context (BSD)
 *
 * Parameter:
 *   key    - Elemen yang dicari
 *   base   - Array awal (harus terurut)
 *   nmemb  - Jumlah elemen
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan dengan context
 *   arg    - User context
 *
 * Return: Pointer ke elemen yang ditemukan, atau NULL
 */
void *bsearch_r(const void *key, const void *base, size_t nmemb,
                size_t size,
                int (*compar)(const void *, const void *, void *),
                void *arg) {
    const unsigned char *arr = (const unsigned char *)base;
    size_t lo, hi, mid;
    int cmp;

    /* Validasi parameter */
    if (key == NULL || base == NULL || nmemb == 0 || size == 0 ||
        compar == NULL) {
        return NULL;
    }

    lo = 0;
    hi = nmemb - 1;

    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;

        cmp = compar(key, arr + mid * size, arg);

        if (cmp == 0) {
            return (void *)(arr + mid * size);
        } else if (cmp < 0) {
            if (mid == 0) {
                break;
            }
            hi = mid - 1;
        } else {
            lo = mid + 1;
        }
    }

    return NULL;
}

/* ============================================================
 * FUNGSI TAMBAHAN (NON-STANDAR)
 * ============================================================
 */

/*
 * lfind - Linear search (tanpa sorted array)
 *
 * Parameter:
 *   key    - Elemen yang dicari
 *   base   - Array awal
 *   nmemb  - Jumlah elemen
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan
 *
 * Return: Pointer ke elemen yang ditemukan, atau NULL
 *
 * Catatan: Berbeda dengan bsearch, lfind tidak memerlukan
 *          array terurut. Kompleksitas O(n).
 */
void *lfind(const void *key, const void *base, size_t *nmemb,
            size_t size, int (*compar)(const void *, const void *)) {
    const unsigned char *arr = (const unsigned char *)base;
    size_t i;
    size_t count;

    /* Validasi parameter */
    if (key == NULL || base == NULL || nmemb == NULL || size == 0 ||
        compar == NULL) {
        return NULL;
    }

    count = *nmemb;

    for (i = 0; i < count; i++) {
        if (compar(key, arr + i * size) == 0) {
            return (void *)(arr + i * size);
        }
    }

    return NULL;
}

/*
 * lsearch - Linear search dengan insert jika tidak ditemukan
 *
 * Parameter:
 *   key    - Elemen yang dicari
 *   base   - Array awal
 *   nmemb  - Pointer ke jumlah elemen (diupdate jika insert)
 *   size   - Ukuran setiap elemen
 *   compar - Fungsi perbandingan
 *
 * Return: Pointer ke elemen yang ditemukan atau yang diinsert
 *
 * Catatan: Jika elemen tidak ditemukan, elemen akan diinsert
 *          di akhir array dan *nmemb diupdate.
 */
void *lsearch(const void *key, void *base, size_t *nmemb,
              size_t size, int (*compar)(const void *, const void *)) {
    unsigned char *arr = (unsigned char *)base;
    size_t i;
    size_t count;

    /* Validasi parameter */
    if (key == NULL || base == NULL || nmemb == NULL || size == 0 ||
        compar == NULL) {
        return NULL;
    }

    count = *nmemb;

    /* Cari dulu dengan linear search */
    for (i = 0; i < count; i++) {
        if (compar(key, arr + i * size) == 0) {
            return (void *)(arr + i * size);
        }
    }

    /* Tidak ditemukan, insert di akhir */
    /* Catatan: Tidak ada pengecekan kapasitas array,
     *          pemanggil harus memastikan array cukup besar */
    for (i = 0; i < size; i++) {
        arr[count * size + i] = ((const unsigned char *)key)[i];
    }

    (*nmemb)++;

    return (void *)(arr + count * size);
}
